#include "pch.h"
#include "ExtraUpscalers_Dx12.h"

#include <Config.h>
#include <fsr4/FSR4DriverProvider.h>
#include <fsr4/FSR4Models.h>
#include <fsr4/FSR4Presets.h>
#include <misc/IdentifyGpu.h>
#include <proxies/KernelBase_Proxy.h>
#include <proxies/Ntdll_Proxy.h>

#include <ffx_upscale.h>

namespace
{
// One upscaler of an extra dll, SDK exports and driver providers have the same signatures
struct Upscaler
{
    uint64_t versionId = 0;
    std::string name;
    HMODULE module = nullptr;

    // Only versions the default dlls don't have are listed and created with this dll. An SDK dll also lists the
    // driver's upscaler, that one stays with Opti's own driver dll.
    bool listed = false;

    PfnFfxCreateContext createContext = nullptr;
    PfnFfxDestroyContext destroyContext = nullptr;
    PfnFfxConfigure configure = nullptr;
    PfnFfxQuery query = nullptr;
    PfnFfxDispatch dispatch = nullptr;
};

std::once_flag scanOnce;
std::vector<std::unique_ptr<Upscaler>> upscalers;
std::vector<std::unique_ptr<FSR4DriverDll>> driverDlls; // Kept for their stand-ins and hooks

std::unordered_map<ffxContext, Upscaler*> contexts;
std::mutex contextsMutex;

// The name and where it's from: the subfolder, or Extra for the folder itself
std::string Label(const std::string& name, const std::filesystem::path& file, const std::filesystem::path& folder)
{
    auto parent = file.parent_path();
    auto source = parent == folder ? std::string("Extra") : wstring_to_string(parent.filename().wstring());

    return std::format("{} ({})", name, source);
}

void AddUpscaler(Upscaler upscaler)
{
    if (upscaler.createContext == nullptr || upscaler.versionId == 0)
        return;

    if (std::ranges::any_of(upscalers, [&](const auto& other) { return other->versionId == upscaler.versionId; }))
    {
        LOG_INFO("Extra upscaler {} skipped, its version id {:X} is already listed", upscaler.name, upscaler.versionId);
        return;
    }

    LOG_INFO("Extra upscaler: {}, id: {:X}", upscaler.name, upscaler.versionId);
    upscalers.push_back(std::make_unique<Upscaler>(std::move(upscaler)));
}

void LoadSdkDll(const std::filesystem::path& file, const std::filesystem::path& folder, void* device)
{
    // Its dependencies come from its own folder
    auto module = NtdllProxy::LoadLibraryExW_Ldr(file.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (module == nullptr)
    {
        LOG_ERROR(L"Can't load {}", file.wstring());
        return;
    }

    Upscaler functions {};
    functions.module = module;
    functions.createContext =
        reinterpret_cast<PfnFfxCreateContext>(KernelBaseProxy::GetProcAddress_()(module, "ffxCreateContext"));
    functions.destroyContext =
        reinterpret_cast<PfnFfxDestroyContext>(KernelBaseProxy::GetProcAddress_()(module, "ffxDestroyContext"));
    functions.configure = reinterpret_cast<PfnFfxConfigure>(KernelBaseProxy::GetProcAddress_()(module, "ffxConfigure"));
    functions.query = reinterpret_cast<PfnFfxQuery>(KernelBaseProxy::GetProcAddress_()(module, "ffxQuery"));
    functions.dispatch = reinterpret_cast<PfnFfxDispatch>(KernelBaseProxy::GetProcAddress_()(module, "ffxDispatch"));

    if (functions.createContext == nullptr || functions.destroyContext == nullptr || functions.configure == nullptr ||
        functions.query == nullptr || functions.dispatch == nullptr)
    {
        LOG_ERROR(L"{} isn't an FFX dll", file.wstring());
        return;
    }

    // Opti's FSR 4 model and preset choices, as for the default FFX dlls
    FSR4Presets::Hook(module);
    FSR4Models::Hook(module);

    ffxQueryDescGetVersions versionQuery {};
    versionQuery.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    versionQuery.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    versionQuery.device = device;

    uint64_t count = 0;
    versionQuery.outputCount = &count;

    if (functions.query(nullptr, &versionQuery.header) != FFX_API_RETURN_OK || count == 0)
    {
        LOG_WARN(L"{} has no upscaler", file.wstring());
        return;
    }

    std::vector<uint64_t> ids(count);
    std::vector<const char*> names(count);
    versionQuery.versionIds = ids.data();
    versionQuery.versionNames = names.data();

    if (functions.query(nullptr, &versionQuery.header) != FFX_API_RETURN_OK)
        return;

    for (size_t i = 0; i < std::min<size_t>(count, ids.size()); i++)
    {
        auto upscaler = functions;
        upscaler.versionId = ids[i];
        upscaler.name = Label(names[i] != nullptr ? names[i] : "FSR", file, folder);
        AddUpscaler(std::move(upscaler));
    }
}

void LoadDriverDll(const std::filesystem::path& file, const std::filesystem::path& folder)
{
    IdentifyGpu::updateD3d12Capabilities();

    if (IdentifyGpu::getPrimaryGpu().fsr4Support == FSR4Support::None)
    {
        LOG_INFO(L"GPU doesn't support FSR 4, skipping {}", file.wstring());
        return;
    }

    auto dll = std::make_unique<FSR4DriverDll>(L"amdxcffx64.dll",
                                               std::vector<std::wstring> { L"amd_fidelityfx_dx12.dll" }, file);

    if (dll->Load() == nullptr)
        return;

    // Version 1 for the first drivers with FSR 4, as the default driver dll
    auto provider = dll->QueryProvider(FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE, { 2u, 1u });

    if (provider.has_value())
    {
        Upscaler upscaler {};
        upscaler.versionId = provider->functions.versionId;
        upscaler.name = Label(provider->versionName.empty() ? "FSR 4" : provider->versionName, file, folder);
        upscaler.module = dll->Module();
        upscaler.createContext = provider->functions.createContext;
        upscaler.destroyContext = provider->functions.destroyContext;
        upscaler.configure = provider->functions.configure;
        upscaler.query = provider->functions.query;
        upscaler.dispatch = provider->functions.dispatch;
        AddUpscaler(std::move(upscaler));
    }

    driverDlls.push_back(std::move(dll));
}

void Scan(void* device)
{
    if (!Config::Instance()->MainDllPath.has_value())
        return;

    const auto folder = std::filesystem::path(Config::Instance()->MainDllPath.value()) / ExtraUpscalersDx12::FolderName;
    std::error_code error;

    if (!std::filesystem::is_directory(folder, error))
    {
        LOG_INFO(L"No extra upscaler dlls, {} doesn't exist", folder.wstring());
        return;
    }

    LOG_INFO(L"Looking for upscaler dlls in {}", folder.wstring());

    // Sorted so the list keeps its order between runs
    std::vector<std::filesystem::path> files;
    for (auto it = std::filesystem::recursive_directory_iterator(folder, error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error))
    {
        if (it->is_regular_file(error))
            files.push_back(it->path());
    }

    std::ranges::sort(files);

    for (const auto& file : files)
    {
        auto name = file.filename().wstring();
        std::ranges::transform(name, name.begin(), ::towlower);

        if (name == L"amd_fidelityfx_upscaler_dx12.dll" || name == L"amd_fidelityfx_dx12.dll")
            LoadSdkDll(file, folder, device);
        else if (name == L"amdxcffx64.dll")
            LoadDriverDll(file, folder);
    }
}

Upscaler* Find(uint64_t versionId)
{
    for (const auto& upscaler : upscalers)
    {
        if (upscaler->versionId == versionId)
            return upscaler.get();
    }

    return nullptr;
}

Upscaler* Owner(ffxContext* context)
{
    if (context == nullptr || *context == nullptr)
        return nullptr;

    std::scoped_lock lock(contextsMutex);
    auto it = contexts.find(*context);
    return it != contexts.end() ? it->second : nullptr;
}
} // namespace

void ExtraUpscalersDx12::AddToVersionList(std::vector<uint64_t>& ids, std::vector<const char*>& names, void* device)
{
    std::call_once(scanOnce, [device] { Scan(device); });

    for (const auto& upscaler : upscalers)
    {
        if (std::ranges::find(ids, upscaler->versionId) != ids.end())
            continue;

        ids.push_back(upscaler->versionId);
        names.push_back(upscaler->name.c_str());

        if (!upscaler->listed)
        {
            upscaler->listed = true;
            LOG_INFO("Listed extra upscaler {}, id: {:X}", upscaler->name, upscaler->versionId);
        }
    }
}

bool ExtraUpscalersDx12::HasVersion(uint64_t versionId)
{
    if (versionId == 0)
        return false;

    auto upscaler = Find(versionId);
    return upscaler != nullptr && upscaler->listed;
}

HMODULE ExtraUpscalersDx12::Module(uint64_t versionId)
{
    auto upscaler = Find(versionId);
    return upscaler != nullptr && upscaler->listed ? upscaler->module : nullptr;
}

bool ExtraUpscalersDx12::IsModule(HMODULE module)
{
    return module != nullptr &&
           std::ranges::any_of(upscalers, [module](const auto& upscaler) { return upscaler->module == module; });
}

bool ExtraUpscalersDx12::OwnsContext(ffxContext* context) { return Owner(context) != nullptr; }

ffxReturnCode_t ExtraUpscalersDx12::CreateContext(uint64_t versionId, ffxContext* context,
                                                  ffxCreateContextDescHeader* desc, const ffxAllocationCallbacks* memCb)
{
    auto upscaler = Find(versionId);
    if (upscaler == nullptr || !upscaler->listed)
        return FFX_API_RETURN_NO_PROVIDER;

    auto result = upscaler->createContext(context, desc, memCb);
    LOG_INFO("Created extra upscaler {} context, result: {}", upscaler->name, (UINT) result);

    if (result == FFX_API_RETURN_OK)
    {
        std::scoped_lock lock(contextsMutex);
        contexts[*context] = upscaler;
    }

    return result;
}

ffxReturnCode_t ExtraUpscalersDx12::DestroyContext(ffxContext* context, const ffxAllocationCallbacks* memCb)
{
    auto upscaler = Owner(context);
    if (upscaler == nullptr)
        return FFX_API_RETURN_ERROR_PARAMETER;

    auto handle = *context;
    auto result = upscaler->destroyContext(context, memCb);

    if (result == FFX_API_RETURN_OK)
    {
        std::scoped_lock lock(contextsMutex);
        contexts.erase(handle);
    }

    return result;
}

ffxReturnCode_t ExtraUpscalersDx12::Configure(ffxContext* context, const ffxConfigureDescHeader* desc)
{
    auto upscaler = Owner(context);
    return upscaler != nullptr ? upscaler->configure(context, desc) : FFX_API_RETURN_ERROR_PARAMETER;
}

ffxReturnCode_t ExtraUpscalersDx12::Query(ffxContext* context, ffxQueryDescHeader* desc)
{
    auto upscaler = Owner(context);
    return upscaler != nullptr ? upscaler->query(context, desc) : FFX_API_RETURN_ERROR_PARAMETER;
}

ffxReturnCode_t ExtraUpscalersDx12::Dispatch(ffxContext* context, const ffxDispatchDescHeader* desc)
{
    auto upscaler = Owner(context);
    return upscaler != nullptr ? upscaler->dispatch(context, desc) : FFX_API_RETURN_ERROR_PARAMETER;
}
