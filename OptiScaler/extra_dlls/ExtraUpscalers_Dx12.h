#pragma once
#include "SysUtils.h"

#include <ffx_api.h>

// More DX12 FFX upscaler dlls from the "Extra dlls" folder in OptiDllPath, also in its subfolders: FFX SDK
// upscaler dlls (amd_fidelityfx_upscaler_dx12.dll), FFX loaders (amd_fidelityfx_dx12.dll) and copies of the driver dll
// (amdxcffx64.dll). Each of their upscalers is listed next to the default ones in the FSR version list of the menu,
// a version that's already listed is skipped. Contexts created with one of them stay with its dll.
namespace ExtraUpscalersDx12
{
// The folder's name in OptiDllPath, .\OptiScaler by default
inline constexpr const wchar_t* FolderName = L"Extra dlls";

// Loads the dlls on the first call and adds their upscalers that ids doesn't have
void AddToVersionList(std::vector<uint64_t>& ids, std::vector<const char*>& names, void* device);

bool HasVersion(uint64_t versionId);

// The dll of the version, nullptr if it isn't an extra one
HMODULE Module(uint64_t versionId);

bool IsModule(HMODULE module);
bool OwnsContext(ffxContext* context);

ffxReturnCode_t CreateContext(uint64_t versionId, ffxContext* context, ffxCreateContextDescHeader* desc,
                              const ffxAllocationCallbacks* memCb);
ffxReturnCode_t DestroyContext(ffxContext* context, const ffxAllocationCallbacks* memCb);
ffxReturnCode_t Configure(ffxContext* context, const ffxConfigureDescHeader* desc);
ffxReturnCode_t Query(ffxContext* context, ffxQueryDescHeader* desc);
ffxReturnCode_t Dispatch(ffxContext* context, const ffxDispatchDescHeader* desc);
} // namespace ExtraUpscalersDx12
