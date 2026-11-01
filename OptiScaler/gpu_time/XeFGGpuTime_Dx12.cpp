#include "pch.h"
#include "XeFGGpuTime_Dx12.h"

#include <Util.h>
#include <misc/DeferredRelease.h>
#include <proxies/XeFG_Proxy.h>

#include <include/d3dx/d3dx12.h>
#include <magic_enum.hpp>

namespace
{
// The callbacks libxess_fg takes, version 4. Versions 2 and 3 are the same struct with 10 and 11 callbacks.
// Only the frame and command list callbacks are used, the others inspect XeFG's inputs and outputs.
typedef void (*PFN_FrameCallback)(void* userData, uint32_t presentId, uint32_t active);
typedef void (*PFN_CommandListCallback)(void* userData, uint64_t unknown, ID3D12GraphicsCommandList* commandList);
typedef void (*PFN_IgnoredCallback)();

struct DiagnosticCallbacks
{
    uint32_t version = 4;
    void* userData = nullptr;
    PFN_FrameCallback frame = nullptr;
    PFN_CommandListCallback beginCommandList = nullptr;
    PFN_CommandListCallback endCommandList = nullptr;
    std::array<PFN_IgnoredCallback, 9> ignored {};
};

static_assert(sizeof(DiagnosticCallbacks) == 0x70);

typedef xefg_swapchain_result_t (*PFN_SetDiagnosticCallbacks)(xefg_swapchain_handle_t context,
                                                              const DiagnosticCallbacks* callbacks);

// Timestamps work on direct and compute lists, copy lists are left untimed
constexpr UINT TypeCount = 2;
constexpr UINT SlotCount = 64;

// A list XeFG never ended frees its slot after this many frames
constexpr UINT64 OpenFrames = 8;

// A list that ended but never ran frees its slot after this many frames
constexpr UINT64 StaleFrames = 120;

// After the timing starts, the lists per frame and their times are logged once
constexpr double DiagnosticsMs = 3000.0;

struct Slot
{
    ID3D12GraphicsCommandList* list = nullptr; // Only compared
    UINT64 frame = 0;
    UINT order = 0; // The list's place in its frame, 0 is the frame generation itself
    UINT type = 0;
    bool open = false;
    bool pending = false;
};

struct Diagnostics
{
    bool active = false;
    double startedAt = 0.0;
    UINT64 frames = 0;
    UINT64 lists = 0;
    UINT64 finished = 0;
    UINT64 firstFinished = 0;
    double firstMs = 0.0;
    double allMs = 0.0;
    UINT untimed = 0;
};

std::mutex timerMutex;
std::atomic<bool> timerActive = false;

PFN_SetDiagnosticCallbacks setCallbacks = nullptr;
bool exportChecked = false;
xefg_swapchain_handle_t registered = nullptr;
xefg_swapchain_handle_t refused = nullptr; // Not retried every frame

ID3D12Device* timerDevice = nullptr;
ID3D12QueryHeap* queryHeap = nullptr;
ID3D12Resource* readback = nullptr;
std::array<Slot, SlotCount> slots {};
std::array<UINT64, TypeCount> frequencies {};
UINT nextSlot = 0;

UINT64 frameNumber = 0;
UINT listsInFrame = 0;

Diagnostics diagnostics;

// Called with timerMutex held
void ReleaseResources()
{
    DeferredRelease::Release(queryHeap);
    DeferredRelease::Release(readback);
    DeferredRelease::Release(timerDevice);

    slots = {};
    frequencies = {};
    nextSlot = 0;
}

// Called with timerMutex held
bool CreateResources(ID3D12Device* device)
{
    D3D12_QUERY_HEAP_DESC heapDesc = {};
    heapDesc.Count = SlotCount * 2;
    heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

    auto result = device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&queryHeap));

    if (result != S_OK)
    {
        LOG_ERROR("CreateQueryHeap error: {:X}", (UINT) result);
        return false;
    }

    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(SlotCount * 2 * sizeof(UINT64));
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;

    result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                             D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback));

    if (result != S_OK)
    {
        LOG_ERROR("CreateCommittedResource error: {:X}", (UINT) result);
        DeferredRelease::Release(queryHeap);
        return false;
    }

    // A finished slot has both timestamps, unfinished ones stay 0
    UINT64* data = nullptr;
    D3D12_RANGE noRead = { 0, 0 };

    if (readback->Map(0, &noRead, reinterpret_cast<void**>(&data)) == S_OK && data != nullptr)
    {
        memset(data, 0, SlotCount * 2 * sizeof(UINT64));
        readback->Unmap(0, nullptr);
    }

    timerDevice = device;
    timerDevice->AddRef();

    return true;
}

// The lists don't tell their queue, a queue of the same type on the device gives the timestamp frequency.
// Called with timerMutex held
UINT64 Frequency(UINT type, D3D12_COMMAND_LIST_TYPE listType)
{
    if (frequencies[type] != 0)
        return frequencies[type];

    D3D12_COMMAND_QUEUE_DESC desc {};
    desc.Type = listType;

    ID3D12CommandQueue* queue = nullptr;
    if (timerDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)) == S_OK && queue != nullptr)
    {
        queue->GetTimestampFrequency(&frequencies[type]);
        queue->Release();
    }

    return frequencies[type];
}

void OnFrame(void* userData, uint32_t presentId, uint32_t active)
{
    if (!timerActive.load(std::memory_order_relaxed))
        return;

    std::scoped_lock lock(timerMutex);

    frameNumber++;
    listsInFrame = 0;

    if (diagnostics.active)
        diagnostics.frames++;

    for (auto& slot : slots)
    {
        if (slot.open && frameNumber - slot.frame > OpenFrames)
            slot = {};
    }
}

void OnBeginCommandList(void* userData, uint64_t unknown, ID3D12GraphicsCommandList* commandList)
{
    if (!timerActive.load(std::memory_order_relaxed) || commandList == nullptr)
        return;

    const auto listType = commandList->GetType();
    UINT type = 0;

    if (listType == D3D12_COMMAND_LIST_TYPE_DIRECT)
        type = 0;
    else if (listType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        type = 1;
    else
        return;

    std::scoped_lock lock(timerMutex);

    const UINT order = listsInFrame++;

    if (diagnostics.active)
        diagnostics.lists++;

    ID3D12Device* device = nullptr;
    if (commandList->GetDevice(IID_PPV_ARGS(&device)) != S_OK || device == nullptr)
        return;

    // The game can recreate its device
    if (device != timerDevice)
    {
        ReleaseResources();

        if (!CreateResources(device))
        {
            device->Release();
            timerActive = false;
            return;
        }
    }

    device->Release();

    if (Frequency(type, listType) == 0)
        return;

    for (UINT i = 0; i < SlotCount; i++)
    {
        const UINT index = (nextSlot + i) % SlotCount;
        auto& slot = slots[index];

        if (slot.open || slot.pending)
            continue;

        slot = { commandList, frameNumber, order, type, true, false };
        nextSlot = (index + 1) % SlotCount;

        commandList->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, index * 2);
        return;
    }

    // Far behind GPU, every slot still runs
    if (diagnostics.active)
        diagnostics.untimed++;
}

void OnEndCommandList(void* userData, uint64_t unknown, ID3D12GraphicsCommandList* commandList)
{
    if (!timerActive.load(std::memory_order_relaxed) || commandList == nullptr)
        return;

    std::scoped_lock lock(timerMutex);

    for (UINT index = 0; index < SlotCount; index++)
    {
        auto& slot = slots[index];

        if (!slot.open || slot.list != commandList)
            continue;

        commandList->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, index * 2 + 1);
        commandList->ResolveQueryData(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, index * 2, 2, readback,
                                      index * 2 * sizeof(UINT64));

        slot.open = false;
        slot.pending = true;
        return;
    }
}

void OnIgnored() {}

// XeFG passes userData back unchanged, the state is global so it's only kept non-null.
// No callback is left null, XeFG calls them without checking
const DiagnosticCallbacks callbacks = {
    4,
    &timerMutex,
    OnFrame,
    OnBeginCommandList,
    OnEndCommandList,
    { OnIgnored, OnIgnored, OnIgnored, OnIgnored, OnIgnored, OnIgnored, OnIgnored, OnIgnored, OnIgnored },
};

// Called with timerMutex held
void LogDiagnostics()
{
    const auto frames = std::max<UINT64>(diagnostics.frames, 1);
    const auto firstFinished = std::max<UINT64>(diagnostics.firstFinished, 1);

    LOG_INFO("XeFG GPU time: {} frames in {:.0f} ms, {:.2f} command lists per frame, {} finished ({} untimed). "
             "First list {:.3f} ms per frame, all lists {:.3f} ms per frame",
             diagnostics.frames, DiagnosticsMs, static_cast<double>(diagnostics.lists) / frames, diagnostics.finished,
             diagnostics.untimed, diagnostics.firstMs / firstFinished, diagnostics.allMs / firstFinished);

    diagnostics = {};
}
} // namespace

bool XeFGGpuTime_Dx12::Enable(xefg_swapchain_handle_t context, bool enable)
{
    if (!enable || context == nullptr)
    {
        if (registered != nullptr)
        {
            timerActive = false;

            if (setCallbacks != nullptr)
                setCallbacks(registered, nullptr);

            registered = nullptr;
            LOG_INFO("XeFG GPU time stopped");
        }

        return false;
    }

    if (!exportChecked)
    {
        exportChecked = true;

        if (auto module = XeFGProxy::Module(); module != nullptr)
        {
            setCallbacks = reinterpret_cast<PFN_SetDiagnosticCallbacks>(
                KernelBaseProxy::GetProcAddress_()(module, "xefgSwapChainD3D12SetDiagnosticCallbacks"));
        }

        if (setCallbacks == nullptr)
            LOG_INFO("This libxess_fg has no diagnostic callbacks, XeFG's submissions are timed instead");
    }

    if (setCallbacks == nullptr || context == refused)
        return false;

    if (context != registered)
    {
        if (registered != nullptr)
            setCallbacks(registered, nullptr);

        {
            std::scoped_lock lock(timerMutex);
            slots = {};
            frameNumber = 0;
            listsInFrame = 0;
            diagnostics = {};
            diagnostics.active = true;
            diagnostics.startedAt = Util::MillisecondsNow();
        }

        timerActive = true;
        auto result = setCallbacks(context, &callbacks);

        if (result != XEFG_SWAPCHAIN_RESULT_SUCCESS)
        {
            LOG_ERROR("SetDiagnosticCallbacks error: {} ({})", magic_enum::enum_name(result), (int) result);
            timerActive = false;
            registered = nullptr;
            refused = context;
            return false;
        }

        registered = context;
        LOG_INFO("XeFG GPU time: diagnostic callbacks registered");
    }

    return timerActive.load();
}

void XeFGGpuTime_Dx12::Forget(xefg_swapchain_handle_t context)
{
    if (context == nullptr)
        return;

    if (registered == context)
    {
        timerActive = false;
        registered = nullptr;
    }

    if (refused == context)
        refused = nullptr;
}

double XeFGGpuTime_Dx12::Collect()
{
    std::scoped_lock lock(timerMutex);

    if (diagnostics.active && Util::MillisecondsNow() - diagnostics.startedAt > DiagnosticsMs)
        LogDiagnostics();

    if (readback == nullptr || std::ranges::none_of(slots, [](const Slot& slot) { return slot.pending; }))
        return 0.0;

    UINT64* data = nullptr;
    D3D12_RANGE readRange = { 0, SlotCount * 2 * sizeof(UINT64) };

    if (readback->Map(0, &readRange, reinterpret_cast<void**>(&data)) != S_OK || data == nullptr)
        return 0.0;

    double firstMs = 0.0;
    size_t writtenEnd = 0;

    for (UINT index = 0; index < SlotCount; index++)
    {
        auto& slot = slots[index];

        if (!slot.pending)
            continue;

        // A list XeFG recorded but never executed would keep its slot forever
        if (frameNumber - slot.frame > StaleFrames)
        {
            data[index * 2] = 0;
            data[index * 2 + 1] = 0;
            writtenEnd = (index + 1) * 2 * sizeof(UINT64);
            slot = {};
            continue;
        }

        if (data[index * 2] == 0 || data[index * 2 + 1] == 0)
            continue;

        const UINT64 start = data[index * 2];
        const UINT64 end = data[index * 2 + 1];
        const UINT64 frequency = frequencies[slot.type];

        // Filters out wrong timestamps, as the upscaler time does
        if (end >= start && frequency != 0)
        {
            const double ms = (end - start) * 1000.0 / static_cast<double>(frequency);

            if (ms < 100.0)
            {
                if (slot.order == 0)
                    firstMs += ms;

                if (diagnostics.active)
                {
                    diagnostics.allMs += ms;

                    if (slot.order == 0)
                    {
                        diagnostics.firstMs += ms;
                        diagnostics.firstFinished++;
                    }
                }
            }
        }

        if (diagnostics.active)
            diagnostics.finished++;

        data[index * 2] = 0;
        data[index * 2 + 1] = 0;
        writtenEnd = (index + 1) * 2 * sizeof(UINT64);
        slot = {};
    }

    D3D12_RANGE writtenRange = { 0, writtenEnd };
    readback->Unmap(0, &writtenRange);

    return firstMs;
}
