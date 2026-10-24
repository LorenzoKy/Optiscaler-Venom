#include "pch.h"
#include "FGGpuTime_Dx12.h"

#include <Util.h>
#include <misc/DeferredRelease.h>

#include <include/d3dx/d3dx12.h>
#include <detours/detours.h>

#include <filesystem>
#include <intrin.h>
#include <map>

namespace
{
typedef void(STDMETHODCALLTYPE* PFN_ExecuteCommandLists)(ID3D12CommandQueue* This, UINT NumCommandLists,
                                                         ID3D12CommandList* const* ppCommandLists);

// ID3D12CommandQueue vtable index of ExecuteCommandLists
constexpr int ExecuteCommandListsIndex = 10;

// Return addresses checked for a frame generation library, the library can call through a wrapper or a layer
constexpr USHORT StackFrames = 16;

// Queues of different types or devices can have their own ExecuteCommandLists
constexpr size_t MaxTargets = 4;

// After the timing starts, which libraries submit is logged once
constexpr double DiagnosticsMs = 3000.0;

// Timestamps work on direct and compute queues, copy queues are left untimed
constexpr UINT TypeCount = 2;
constexpr UINT SlotsPerType = 64;
constexpr UINT SlotCount = SlotsPerType * TypeCount;

// One timed submission: a list with the start timestamp before the library's lists and one with the end timestamp
// and its resolve after them. Recorded once, executed again when the last execution finished.
struct Slot
{
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12GraphicsCommandList* begin = nullptr;
    ID3D12GraphicsCommandList* end = nullptr;
    bool pending = false;
};

struct ModuleRange
{
    uintptr_t begin = 0;
    uintptr_t end = 0;
};

// The hooked functions and their trampolines, hooks[i] calls originals[i]
std::array<void*, MaxTargets> targets {};
std::array<PFN_ExecuteCommandLists, MaxTargets> originals {};
size_t targetCount = 0;
std::mutex hookMutex;
ID3D12Device* hookedDevice = nullptr; // Only compared, the queues of its device are hooked

// A submission through a layer reaches the next hooked function on the same thread, it's timed once
thread_local bool inTimedSubmit = false;

// Libraries in the stacks of the submissions, their count and how many were timed
struct Diagnostics
{
    bool active = false;
    double startedAt = 0.0;
    UINT submissions = 0;
    UINT matched = 0;
    UINT timed = 0;
    UINT finished = 0;
    std::map<std::wstring, UINT> modules;
};

Diagnostics diagnostics;

std::mutex timerMutex;
std::atomic<bool> timerActive = false;
std::vector<ModuleRange> modules;

ID3D12Device* timerDevice = nullptr;
ID3D12QueryHeap* queryHeap = nullptr;
ID3D12Resource* readback = nullptr;
std::array<Slot, SlotCount> slots {};
std::array<UINT, TypeCount> nextSlot {};
std::array<UINT64, TypeCount> frequencies {};

void ReleaseResources()
{
    for (auto& slot : slots)
    {
        DeferredRelease::Release(slot.begin);
        DeferredRelease::Release(slot.end);
        DeferredRelease::Release(slot.allocator);
        slot.pending = false;
    }

    DeferredRelease::Release(queryHeap);
    DeferredRelease::Release(readback);
    DeferredRelease::Release(timerDevice);

    nextSlot = {};
    frequencies = {};
}

// The query heap and the readback buffer for the device of the queue, the slots' lists are created when first used
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

bool RecordSlot(Slot& slot, UINT index, D3D12_COMMAND_LIST_TYPE type)
{
    auto result = timerDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&slot.allocator));

    if (result != S_OK)
    {
        LOG_ERROR("CreateCommandAllocator error: {:X}", (UINT) result);
        return false;
    }

    // An allocator records one list at a time, the first is closed before the second is created
    result = timerDevice->CreateCommandList(0, type, slot.allocator, nullptr, IID_PPV_ARGS(&slot.begin));

    if (result == S_OK)
    {
        slot.begin->SetName(L"FGGpuTime begin");
        slot.begin->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, index * 2);
        slot.begin->Close();

        result = timerDevice->CreateCommandList(0, type, slot.allocator, nullptr, IID_PPV_ARGS(&slot.end));
    }

    if (result != S_OK)
    {
        LOG_ERROR("CreateCommandList error: {:X}", (UINT) result);
        DeferredRelease::Release(slot.begin);
        DeferredRelease::Release(slot.end);
        DeferredRelease::Release(slot.allocator);
        return false;
    }

    slot.end->SetName(L"FGGpuTime end");
    slot.end->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, index * 2 + 1);
    slot.end->ResolveQueryData(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, index * 2, 2, readback,
                               index * 2 * sizeof(UINT64));
    slot.end->Close();

    return true;
}

// Opti's own submissions (its overlay, UI and latency lists) can run inside the frame generation's present
ModuleRange optiRange {};

ModuleRange RangeOf(HMODULE module)
{
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(module) + dos->e_lfanew);
    auto begin = reinterpret_cast<uintptr_t>(module);

    return { begin, begin + nt->OptionalHeader.SizeOfImage };
}

// The stack also has the hook's own frames, they're in Opti and never match a frame generation library
bool FromModules(void* caller, void* const* frames, USHORT frameCount)
{
    const auto callerAddress = reinterpret_cast<uintptr_t>(caller);
    if (callerAddress >= optiRange.begin && callerAddress < optiRange.end)
        return false;

    auto inModules = [](uintptr_t address)
    {
        return std::ranges::any_of(modules, [address](const ModuleRange& range)
                                   { return address >= range.begin && address < range.end; });
    };

    if (inModules(callerAddress))
        return true;

    for (USHORT i = 0; i < frameCount; i++)
    {
        if (inModules(reinterpret_cast<uintptr_t>(frames[i])))
            return true;
    }

    return false;
}

std::wstring ModuleName(void* address)
{
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(address), &module))
    {
        return L"?";
    }

    wchar_t path[MAX_PATH] {};
    GetModuleFileNameW(module, path, MAX_PATH);
    return std::filesystem::path(path).filename().wstring();
}

// Called with timerMutex held
void RecordSubmission(void* const* frames, USHORT frameCount, bool matched)
{
    diagnostics.submissions++;

    if (matched)
        diagnostics.matched++;

    std::vector<std::wstring> names;
    for (USHORT i = 0; i < frameCount; i++)
    {
        auto name = ModuleName(frames[i]);

        if (std::ranges::find(names, name) == names.end())
            names.push_back(std::move(name));
    }

    for (const auto& name : names)
        diagnostics.modules[name]++;
}

// Called with timerMutex held
void LogDiagnostics()
{
    std::wstring modules;
    for (const auto& [name, count] : diagnostics.modules)
        modules += std::format(L"{}{} ({})", modules.empty() ? L"" : L", ", name, count);

    LOG_INFO(L"FG GPU time: {} submissions in {:.0f} ms, {} from the frame generation's libraries, {} timed, {} "
             L"finished. Libraries in their stacks: {}",
             diagnostics.submissions, DiagnosticsMs, diagnostics.matched, diagnostics.timed, diagnostics.finished,
             modules);

    diagnostics = {};
}

// A free slot for the queue, nullptr when the submission can't be timed. Called with timerMutex held
Slot* AcquireSlot(ID3D12CommandQueue* queue)
{
    const auto desc = queue->GetDesc();
    UINT type = 0;

    if (desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
        type = 0;
    else if (desc.Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        type = 1;
    else
        return nullptr;

    ID3D12Device* device = nullptr;
    if (queue->GetDevice(IID_PPV_ARGS(&device)) != S_OK || device == nullptr)
        return nullptr;

    // The game can recreate its device
    if (device != timerDevice)
    {
        ReleaseResources();

        if (!CreateResources(device))
        {
            device->Release();
            return nullptr;
        }
    }

    device->Release();

    if (frequencies[type] == 0)
        queue->GetTimestampFrequency(&frequencies[type]);

    // Far behind GPU or not, a submission isn't timed when every slot still runs
    for (UINT i = 0; i < SlotsPerType; i++)
    {
        const UINT index = type * SlotsPerType + (nextSlot[type] + i) % SlotsPerType;
        auto& slot = slots[index];

        if (slot.pending)
            continue;

        // A slot that can't be recorded stops the timing instead of failing on every submission
        if (slot.begin == nullptr && !RecordSlot(slot, index, desc.Type))
        {
            timerActive = false;
            return nullptr;
        }

        nextSlot[type] = (index - type * SlotsPerType + 1) % SlotsPerType;
        slot.pending = true;
        return &slot;
    }

    return nullptr;
}

// caller is the return address of the hook, the stack walk can't tell the hook's frames from Opti's own calls
void ExecuteCommandLists(PFN_ExecuteCommandLists original, void* caller, ID3D12CommandQueue* This, UINT NumCommandLists,
                         ID3D12CommandList* const* ppCommandLists)
{
    if (!timerActive.load(std::memory_order_relaxed) || inTimedSubmit || NumCommandLists == 0 || This == nullptr)
        return original(This, NumCommandLists, ppCommandLists);

    void* frames[StackFrames] {};
    const USHORT frameCount = RtlCaptureStackBackTrace(1, StackFrames, frames, nullptr);

    Slot* slot = nullptr;

    {
        std::scoped_lock lock(timerMutex);

        const bool matched = FromModules(caller, frames, frameCount);

        if (matched)
            slot = AcquireSlot(This);

        if (diagnostics.active)
        {
            RecordSubmission(frames, frameCount, matched);

            if (slot != nullptr)
                diagnostics.timed++;
        }
    }

    if (slot == nullptr)
        return original(This, NumCommandLists, ppCommandLists);

    std::vector<ID3D12CommandList*> lists;
    lists.reserve(NumCommandLists + 2);
    lists.push_back(slot->begin);
    lists.insert(lists.end(), ppCommandLists, ppCommandLists + NumCommandLists);
    lists.push_back(slot->end);

    inTimedSubmit = true;
    original(This, static_cast<UINT>(lists.size()), lists.data());
    inTimedSubmit = false;
}

template <size_t N>
void STDMETHODCALLTYPE hkExecuteCommandLists(ID3D12CommandQueue* This, UINT NumCommandLists,
                                             ID3D12CommandList* const* ppCommandLists)
{
    ExecuteCommandLists(originals[N], _ReturnAddress(), This, NumCommandLists, ppCommandLists);
}

constexpr std::array<PFN_ExecuteCommandLists, MaxTargets> hooks = { hkExecuteCommandLists<0>, hkExecuteCommandLists<1>,
                                                                    hkExecuteCommandLists<2>,
                                                                    hkExecuteCommandLists<3> };

// Hooks the queue's ExecuteCommandLists if it isn't yet. Called with hookMutex held
void HookQueue(ID3D12CommandQueue* queue, const char* from)
{
    auto target = (*reinterpret_cast<void***>(queue))[ExecuteCommandListsIndex];

    if (std::find(targets.begin(), targets.begin() + targetCount, target) != targets.begin() + targetCount)
        return;

    if (targetCount == MaxTargets)
    {
        LOG_WARN("Can't hook another ExecuteCommandLists for the frame generation GPU time");
        return;
    }

    originals[targetCount] = reinterpret_cast<PFN_ExecuteCommandLists>(target);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<PVOID*>(&originals[targetCount]), reinterpret_cast<PVOID>(hooks[targetCount]));

    if (auto result = DetourTransactionCommit(); result != NO_ERROR)
    {
        LOG_ERROR("Hooking ExecuteCommandLists of the {} failed: {}", from, result);
        originals[targetCount] = nullptr;
        return;
    }

    targets[targetCount] = target;
    targetCount++;

    LOG_INFO("Hooked ExecuteCommandLists of the {} for the frame generation GPU time ({:X})", from,
             reinterpret_cast<uintptr_t>(target));
}
} // namespace

void FGGpuTime_Dx12::Hook(ID3D12CommandQueue* queue)
{
    if (queue == nullptr)
        return;

    std::scoped_lock lock(hookMutex);

    if (optiRange.begin == 0)
    {
        HMODULE opti = nullptr;
        if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR>(&ExecuteCommandLists), &opti))
        {
            optiRange = RangeOf(opti);
        }
    }

    HookQueue(queue, "game's queue");

    // The frame generation can submit to its own direct and compute queues, which can have their own function
    ID3D12Device* device = nullptr;
    if (queue->GetDevice(IID_PPV_ARGS(&device)) != S_OK || device == nullptr)
        return;

    if (device != hookedDevice)
    {
        hookedDevice = device;

        for (auto type : { D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_COMPUTE })
        {
            D3D12_COMMAND_QUEUE_DESC desc {};
            desc.Type = type;

            ID3D12CommandQueue* other = nullptr;
            if (device->CreateCommandQueue(&desc, IID_PPV_ARGS(&other)) == S_OK && other != nullptr)
            {
                HookQueue(other, type == D3D12_COMMAND_LIST_TYPE_DIRECT ? "direct queue" : "compute queue");
                other->Release();
            }
        }
    }

    device->Release();
}

void FGGpuTime_Dx12::SetModules(const std::vector<HMODULE>& libraries)
{
    std::vector<ModuleRange> ranges;

    for (auto library : libraries)
    {
        if (library == nullptr)
            continue;

        const auto range = RangeOf(library);

        if (std::ranges::none_of(ranges, [&range](const ModuleRange& other) { return other.begin == range.begin; }))
            ranges.push_back(range);
    }

    bool hooked = false;
    {
        std::scoped_lock lock(hookMutex);
        hooked = targetCount != 0;
    }

    std::scoped_lock lock(timerMutex);
    const bool wasActive = timerActive;
    modules = std::move(ranges);
    timerActive = hooked && !modules.empty();

    if (timerActive && !wasActive)
    {
        LOG_INFO("Timing the GPU work of {} frame generation libraries", modules.size());
        diagnostics = {};
        diagnostics.active = true;
        diagnostics.startedAt = Util::MillisecondsNow();
    }
    else if (!timerActive && wasActive)
    {
        diagnostics = {};
    }
}

double FGGpuTime_Dx12::Collect()
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

    double finishedMs = 0.0;
    size_t writtenEnd = 0;

    for (UINT index = 0; index < SlotCount; index++)
    {
        auto& slot = slots[index];

        if (!slot.pending || data[index * 2] == 0 || data[index * 2 + 1] == 0)
            continue;

        const UINT64 start = data[index * 2];
        const UINT64 end = data[index * 2 + 1];
        const UINT64 frequency = frequencies[index / SlotsPerType];

        // Filters out wrong timestamps, as the upscaler time does
        if (end >= start && frequency != 0)
        {
            const double ms = (end - start) * 1000.0 / static_cast<double>(frequency);

            if (ms < 100.0)
                finishedMs += ms;
        }

        if (diagnostics.active)
            diagnostics.finished++;

        data[index * 2] = 0;
        data[index * 2 + 1] = 0;
        writtenEnd = (index + 1) * 2 * sizeof(UINT64);
        slot.pending = false;
    }

    D3D12_RANGE writtenRange = { 0, writtenEnd };
    readback->Unmap(0, &writtenRange);

    return finishedMs;
}
