#pragma once

#include "pch.h"
#include <State.h>
#include <misc/IdentifyGpu.h>
#include <nvapi/NvApiTypes.h>
#include <proxies/Dxgi_Proxy.h>
#include <proxies/KernelBase_Proxy.h>
#include <proxies/Ntdll_Proxy.h>

#include <psapi.h>
#include <intrin.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace OverlayTelemetry
{
struct GpuMetrics
{
    std::optional<double> tempC;
    std::optional<double> hotspotC; // The hottest sensor: AMD junction, NVIDIA hot spot
    std::optional<double> powerW;
    std::optional<double> voltageV;
    std::optional<uint32_t> coreClockMhz;
    std::optional<uint32_t> memClockMhz;
    std::optional<double> usagePercent; // The busiest engine, as Task Manager shows it

    bool hasAny() const
    {
        return tempC.has_value() || hotspotC.has_value() || powerW.has_value() || voltageV.has_value() ||
               coreClockMhz.has_value() || memClockMhz.has_value() || usagePercent.has_value();
    }
};

struct CpuMetrics
{
    std::optional<double> tempC;    // Package, or AMD's Tctl
    std::optional<double> hotspotC; // The hottest core or CCD
    std::optional<double> powerW;
    std::optional<double> voltageV;
    std::optional<uint32_t> clockMhz;
    std::optional<double> usagePercent;

    bool hasAny() const
    {
        return tempC.has_value() || hotspotC.has_value() || powerW.has_value() || voltageV.has_value() ||
               clockMhz.has_value() || usagePercent.has_value();
    }
};

struct RamMetrics
{
    uint64_t usedBytes = 0;
    uint64_t totalBytes = 0;
    uint64_t gameBytes = 0;
    uint64_t commitUsedBytes = 0;
    uint64_t commitTotalBytes = 0;
};

namespace detail
{
inline GpuInformation ActiveGpuInfo(State& state)
{
    static void* lastD3D12 = nullptr;
    static void* lastD3D11 = nullptr;
    static GpuInformation cachedInfo {};
    static bool hasCached = false;

    if (hasCached && state.currentD3D12Device == lastD3D12 && state.currentD3D11Device == lastD3D11)
        return cachedInfo;

    lastD3D12 = state.currentD3D12Device;
    lastD3D11 = state.currentD3D11Device;

    LUID luid {};
    bool hasDeviceLuid = false;

    if (state.currentD3D12Device != nullptr)
    {
        luid = state.currentD3D12Device->GetAdapterLuid();
        hasDeviceLuid = true;
    }
    else if (state.currentD3D11Device != nullptr)
    {
        IDXGIDevice* dxgiDevice = nullptr;
        IDXGIAdapter* adapter = nullptr;
        DXGI_ADAPTER_DESC desc {};

        if (state.currentD3D11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)) == S_OK &&
            dxgiDevice->GetAdapter(&adapter) == S_OK && adapter->GetDesc(&desc) == S_OK)
        {
            luid = desc.AdapterLuid;
            hasDeviceLuid = true;
        }

        if (adapter != nullptr)
            adapter->Release();

        if (dxgiDevice != nullptr)
            dxgiDevice->Release();
    }

    if (hasDeviceLuid)
    {
        for (const auto& gpu : IdentifyGpu::getAllGpus())
        {
            if (IsEqualLUID(gpu.luid, luid))
            {
                cachedInfo = gpu;
                hasCached = true;
                return cachedInfo;
            }
        }

        GpuInformation info {};
        info.luid = luid;
        cachedInfo = info;
        hasCached = true;
        return cachedInfo;
    }

    cachedInfo = IdentifyGpu::getPrimaryGpu();
    hasCached = true;
    return cachedInfo;
}

inline std::string NormalizeSpaces(std::string s)
{
    auto eraseAll = [&](std::string_view token)
    {
        for (size_t pos = s.find(token); pos != std::string::npos; pos = s.find(token, pos))
            s.erase(pos, token.size());
    };

    eraseAll("(R)");
    eraseAll("(TM)");
    eraseAll("(tm)");
    eraseAll("(r)");

    std::string out;
    out.reserve(s.size());
    bool prevSpace = true;

    for (unsigned char c : s)
    {
        if (std::isspace(c))
        {
            if (!prevSpace)
            {
                out.push_back(' ');
                prevSpace = true;
            }
        }
        else
        {
            out.push_back(static_cast<char>(c));
            prevSpace = false;
        }
    }

    if (!out.empty() && out.back() == ' ')
        out.pop_back();

    return out;
}

inline void StripPrefixIgnoreCase(std::string& s, std::string_view prefix)
{
    if (s.size() >= prefix.size() && _strnicmp(s.data(), prefix.data(), prefix.size()) == 0)
        s.erase(0, prefix.size());
}

inline void StripSuffixIgnoreCase(std::string& s, std::string_view suffix)
{
    if (s.size() >= suffix.size() && _strnicmp(s.data() + s.size() - suffix.size(), suffix.data(), suffix.size()) == 0)
        s.erase(s.size() - suffix.size());
}

inline std::string RawCpuBrandString()
{
    int regs[4] {};
    __cpuid(regs, 0x80000000);
    if (static_cast<uint32_t>(regs[0]) < 0x80000004u)
        return {};

    char brand[49] {};
    __cpuid(reinterpret_cast<int*>(brand), 0x80000002);
    __cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
    __cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
    brand[48] = '\0';

    return std::string(brand);
}
} // namespace detail

inline std::string GetGpuLabel(State& state, bool compact)
{
    const auto gpuInfo = detail::ActiveGpuInfo(state);
    std::string s = detail::NormalizeSpaces(gpuInfo.name);
    if (s.empty())
        return "GPU";

    detail::StripSuffixIgnoreCase(s, " Series");
    detail::StripSuffixIgnoreCase(s, " Graphics");
    detail::StripSuffixIgnoreCase(s, " Laptop GPU");

    if (!compact)
        return s.empty() ? "GPU" : s;

    for (std::string_view prefix : { "NVIDIA GeForce ", "NVIDIA ", "AMD Radeon ", "AMD ", "Intel Arc ", "Intel " })
        detail::StripPrefixIgnoreCase(s, prefix);

    s = detail::NormalizeSpaces(s);
    return s.empty() ? "GPU" : s;
}

inline std::string GetCpuLabel(bool compact)
{
    static const std::string fullLabel = []()
    {
        std::string s = detail::NormalizeSpaces(detail::RawCpuBrandString());
        if (s.empty())
            return std::string("CPU");

        if (auto atPos = s.find(" @"); atPos != std::string::npos)
            s.erase(atPos);

        for (std::string_view marker : { " with ", " w/ " })
        {
            for (size_t i = 0; i + marker.size() <= s.size(); ++i)
            {
                if (_strnicmp(s.data() + i, marker.data(), marker.size()) == 0)
                {
                    s.erase(i);
                    break;
                }
            }
        }

        detail::StripSuffixIgnoreCase(s, " Processor");

        // Strip trailing "<N>-Core"
        if (s.size() > 6 && _stricmp(s.data() + s.size() - 5, "-Core") == 0)
        {
            size_t pos = s.size() - 5;
            while (pos > 0 && std::isdigit(static_cast<unsigned char>(s[pos - 1])))
                --pos;
            if (pos > 0 && s[pos - 1] == ' ')
                s.erase(pos - 1);
        }

        for (std::string_view prefix : { "11th Gen ", "12th Gen ", "13th Gen ", "14th Gen ", "15th Gen ", "16th Gen " })
        {
            detail::StripPrefixIgnoreCase(s, prefix);
        }

        detail::StripSuffixIgnoreCase(s, " CPU");
        s = detail::NormalizeSpaces(s);
        return s.empty() ? std::string("CPU") : s;
    }();

    static const std::string compactLabel = []()
    {
        std::string s = fullLabel;
        if (s == "CPU")
            return s;

        for (std::string_view prefix :
             { "AMD Ryzen Threadripper PRO ", "AMD Ryzen Threadripper ", "AMD Ryzen AI Max+ ", "AMD Ryzen AI Max ",
               "AMD Ryzen AI 9 ", "AMD Ryzen AI 7 ", "AMD Ryzen AI 5 ", "AMD Ryzen 9 ", "AMD Ryzen 7 ", "AMD Ryzen 5 ",
               "AMD Ryzen 3 ", "AMD Ryzen ", "AMD ", "Intel Core Ultra 9 ", "Intel Core Ultra 7 ",
               "Intel Core Ultra 5 ", "Intel Core Ultra 3 ", "Intel Core ", "Intel " })
        {
            detail::StripPrefixIgnoreCase(s, prefix);
        }

        for (std::string_view tier : { "i9-", "i7-", "i5-", "i3-" })
            detail::StripPrefixIgnoreCase(s, tier);

        s = detail::NormalizeSpaces(s);
        return s.empty() ? std::string("CPU") : s;
    }();

    return compact ? compactLabel : fullLabel;
}

namespace detail
{

// Logs the first time a source gives a value, so a log says where the overlay's numbers come from
inline void LogSource(const char* what, const char* source)
{
    static std::mutex mutex;
    static std::vector<std::string> logged;

    std::string entry = std::string(what) + " from " + source;
    std::lock_guard lock(mutex);

    if (std::find(logged.begin(), logged.end(), entry) != logged.end())
        return;

    logged.push_back(entry);
    LOG_INFO("FPS overlay: {}", entry);
}

// AMD Display Library (atiadlxx.dll in System32)
namespace Adl
{
constexpr int ADL_OK = 0;
constexpr int ADL_MAX_PATH = 256;
constexpr int ADL_PMLOG_MAX_SENSORS = 256;

// ADL PMLog sensor IDs
constexpr int SENSOR_CLK_GFXCLK = 1;
constexpr int SENSOR_CLK_MEMCLK = 2;
constexpr int SENSOR_TEMP_EDGE = 8;
constexpr int SENSOR_GFX_VOLTAGE = 21;
constexpr int SENSOR_ASIC_POWER = 23;
constexpr int SENSOR_TEMP_HOTSPOT = 27;
constexpr int SENSOR_SOC_VOLTAGE = 29;
constexpr int SENSOR_GFX_POWER = 30;
constexpr int SENSOR_TEMP_SOC = 31;
constexpr int SENSOR_TEMP_CPU = 32;
constexpr int SENSOR_CPU_POWER = 33;
constexpr int SENSOR_CLK_CPUCLK = 34;
constexpr int SENSOR_SSPAIRED_ASICPOWER = 46;
constexpr int SENSOR_BOARD_POWER = 73;

struct AdapterInfo
{
    int iSize;
    int iAdapterIndex;
    char strUDID[ADL_MAX_PATH];
    int iBusNumber;
    int iDeviceNumber;
    int iFunctionNumber;
    int iVendorID;
    char strAdapterName[ADL_MAX_PATH];
    char strDisplayName[ADL_MAX_PATH];
    int iPresent;
    int iExist;
    char strDriverPath[ADL_MAX_PATH];
    char strDriverPathExt[ADL_MAX_PATH];
    char strPNPString[ADL_MAX_PATH];
    int iOSDisplayIndex;
};

struct ADLPMLogSensor
{
    int supported;
    int value;
};

struct ADLPMLogDataOutput
{
    int size;
    ADLPMLogSensor sensors[ADL_PMLOG_MAX_SENSORS];
};

using MallocCallback = void*(__stdcall*) (int);
using PFN_ADL2_Main_Control_Create = int(__cdecl*)(MallocCallback, int, void**);
using PFN_ADL2_Main_Control_Destroy = int(__cdecl*)(void*);
using PFN_ADL2_Adapter_NumberOfAdapters_Get = int(__cdecl*)(void*, int*);
using PFN_ADL2_Adapter_AdapterInfo_Get = int(__cdecl*)(void*, AdapterInfo*, int);
using PFN_ADL2_New_QueryPMLogData_Get = int(__cdecl*)(void*, int, ADLPMLogDataOutput*);

inline void* __stdcall AdlMalloc(int size) { return std::malloc(static_cast<size_t>(size)); }

struct State
{
    bool initialized = false;
    HMODULE module = nullptr;
    void* context = nullptr;
    int adapterIndex = -1;
    uint32_t matchedDeviceId = 0;
    PFN_ADL2_Adapter_NumberOfAdapters_Get getNumAdapters = nullptr;
    PFN_ADL2_Adapter_AdapterInfo_Get getAdapterInfo = nullptr;
    PFN_ADL2_New_QueryPMLogData_Get getPmLog = nullptr;
    ADLPMLogDataOutput lastPmLog {};
    uint64_t lastQueryTick = 0;
};

inline State& Get()
{
    static State s;
    return s;
}

inline bool PollPmLog(uint32_t targetDeviceId, ADLPMLogDataOutput& outLog)
{
    auto& s = Get();
    const uint64_t nowTick = GetTickCount64();

    if (s.initialized && s.context != nullptr && nowTick - s.lastQueryTick < 250 && s.lastPmLog.size != 0)
    {
        outLog = s.lastPmLog;
        return true;
    }

    if (!s.initialized)
    {
        s.initialized = true;
        s.module = KernelBaseProxy::GetModuleHandleW_()(L"atiadlxx.dll");
        if (s.module == nullptr)
            s.module = NtdllProxy::LoadLibraryExW_Ldr(L"atiadlxx.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

        if (s.module != nullptr)
        {
            auto create = reinterpret_cast<PFN_ADL2_Main_Control_Create>(
                KernelBaseProxy::GetProcAddress_()(s.module, "ADL2_Main_Control_Create"));
            s.getNumAdapters = reinterpret_cast<PFN_ADL2_Adapter_NumberOfAdapters_Get>(
                KernelBaseProxy::GetProcAddress_()(s.module, "ADL2_Adapter_NumberOfAdapters_Get"));
            s.getAdapterInfo = reinterpret_cast<PFN_ADL2_Adapter_AdapterInfo_Get>(
                KernelBaseProxy::GetProcAddress_()(s.module, "ADL2_Adapter_AdapterInfo_Get"));
            s.getPmLog = reinterpret_cast<PFN_ADL2_New_QueryPMLogData_Get>(
                KernelBaseProxy::GetProcAddress_()(s.module, "ADL2_New_QueryPMLogData_Get"));

            if (create == nullptr || s.getPmLog == nullptr || create(AdlMalloc, 1, &s.context) != ADL_OK)
                s.context = nullptr;
        }
    }

    if (s.context == nullptr || s.getPmLog == nullptr)
        return false;

    if (s.adapterIndex < 0 || s.matchedDeviceId != targetDeviceId)
    {
        s.adapterIndex = -1;
        s.matchedDeviceId = targetDeviceId;

        int numAdapters = 0;
        if (s.getNumAdapters != nullptr && s.getAdapterInfo != nullptr &&
            s.getNumAdapters(s.context, &numAdapters) == ADL_OK && numAdapters > 0 && numAdapters <= 64)
        {
            std::vector<AdapterInfo> infos(static_cast<size_t>(numAdapters));
            if (s.getAdapterInfo(s.context, infos.data(), static_cast<int>(infos.size() * sizeof(AdapterInfo))) ==
                ADL_OK)
            {
                char devTag[16] {};
                if (targetDeviceId != 0)
                    std::snprintf(devTag, sizeof(devTag), "DEV_%04X", targetDeviceId & 0xFFFF);

                int fallbackIndex = -1;
                for (const auto& info : infos)
                {
                    ADLPMLogDataOutput testLog {};
                    testLog.size = sizeof(testLog);
                    if (s.getPmLog(s.context, info.iAdapterIndex, &testLog) != ADL_OK)
                        continue;

                    if (fallbackIndex < 0)
                        fallbackIndex = info.iAdapterIndex;

                    if (devTag[0] != '\0' &&
                        (strstr(info.strPNPString, devTag) != nullptr || strstr(info.strUDID, devTag) != nullptr))
                    {
                        s.adapterIndex = info.iAdapterIndex;
                        break;
                    }
                }

                if (s.adapterIndex < 0)
                    s.adapterIndex = fallbackIndex;
            }
        }

        if (s.adapterIndex < 0)
            s.adapterIndex = 0;
    }

    ADLPMLogDataOutput pmLog {};
    pmLog.size = sizeof(pmLog);
    if (s.getPmLog(s.context, s.adapterIndex, &pmLog) != ADL_OK)
        return false;

    s.lastPmLog = pmLog;
    s.lastQueryTick = nowTick;
    outLog = pmLog;
    return true;
}

inline std::optional<int> ReadSensor(const ADLPMLogDataOutput& log, int id)
{
    if (id >= 0 && id < ADL_PMLOG_MAX_SENSORS && log.sensors[id].supported != 0)
        return log.sensors[id].value;

    return std::nullopt;
}
} // namespace Adl

// NVIDIA Management Library (nvml.dll in System32) + NVAPI (nvapi64.dll in System32)
namespace Nvidia
{
using nvmlDevice_t = void*;
using PFN_nvmlInit_v2 = int(__cdecl*)();
using PFN_nvmlDeviceGetCount_v2 = int(__cdecl*)(unsigned int*);
using PFN_nvmlDeviceGetHandleByIndex_v2 = int(__cdecl*)(unsigned int, nvmlDevice_t*);
using PFN_nvmlDeviceGetTemperature = int(__cdecl*)(nvmlDevice_t, int, unsigned int*);
using PFN_nvmlDeviceGetPowerUsage = int(__cdecl*)(nvmlDevice_t, unsigned int*);
using PFN_nvmlDeviceGetClockInfo = int(__cdecl*)(nvmlDevice_t, int, unsigned int*);

struct NvGpuVoltageStatus
{
    NvU32 version;
    NvU32 flags;
    NvU32 reserved[8];
    NvU32 value_uV;
    NvU32 pad[8];
};

using PFN_NvAPI_GPU_GetCurrentVoltage = NvAPI_Status(__cdecl*)(NvPhysicalGpuHandle, NvGpuVoltageStatus*);

// NvAPI_GPU_ThermalGetSensors, not in the public NVAPI: the GPU's sensors in 1/256 C, the hot spot is the second.
// RTX 50 GPUs don't report it anymore.
struct NvThermalSensors
{
    NvU32 version;
    NvU32 mask; // The sensors asked for
    NvS32 reserved[8];
    NvS32 temperatures[32];
};

using PFN_NvAPI_GPU_ThermalGetSensors = NvAPI_Status(__cdecl*)(NvPhysicalGpuHandle, NvThermalSensors*);
constexpr NvU32 NvThermalSensorsVersion = sizeof(NvThermalSensors) | (2u << 16);
constexpr int NvHotspotSensor = 1;

struct State
{
    bool initialized = false;
    nvmlDevice_t nvmlDevice = nullptr;
    PFN_nvmlDeviceGetTemperature getTemp = nullptr;
    PFN_nvmlDeviceGetPowerUsage getPower = nullptr;
    PFN_nvmlDeviceGetClockInfo getClock = nullptr;

    NvPhysicalGpuHandle nvapiGpu = nullptr;
    decltype(&NvAPI_GPU_GetAllClockFrequencies) getAllClocks = nullptr;
    decltype(&NvAPI_GPU_GetThermalSettings) getThermal = nullptr;
    PFN_NvAPI_GPU_GetCurrentVoltage getCurrentVoltage = nullptr;
    PFN_NvAPI_GPU_ThermalGetSensors getThermalSensors = nullptr;
    NvU32 thermalSensorMask = 0; // The sensors the GPU has, found on the first query
};

inline void Query(const GpuInformation& gpuInfo, GpuMetrics& out)
{
    static State s;

    if (!s.initialized)
    {
        s.initialized = true;

        HMODULE nvml = KernelBaseProxy::GetModuleHandleW_()(L"nvml.dll");
        if (nvml == nullptr)
            nvml = NtdllProxy::LoadLibraryExW_Ldr(L"nvml.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

        if (nvml != nullptr)
        {
            auto init = reinterpret_cast<PFN_nvmlInit_v2>(KernelBaseProxy::GetProcAddress_()(nvml, "nvmlInit_v2"));
            auto getHandle = reinterpret_cast<PFN_nvmlDeviceGetHandleByIndex_v2>(
                KernelBaseProxy::GetProcAddress_()(nvml, "nvmlDeviceGetHandleByIndex_v2"));
            s.getTemp = reinterpret_cast<PFN_nvmlDeviceGetTemperature>(
                KernelBaseProxy::GetProcAddress_()(nvml, "nvmlDeviceGetTemperature"));
            s.getPower = reinterpret_cast<PFN_nvmlDeviceGetPowerUsage>(
                KernelBaseProxy::GetProcAddress_()(nvml, "nvmlDeviceGetPowerUsage"));
            s.getClock = reinterpret_cast<PFN_nvmlDeviceGetClockInfo>(
                KernelBaseProxy::GetProcAddress_()(nvml, "nvmlDeviceGetClockInfo"));

            if (init != nullptr && getHandle != nullptr && init() == 0)
                getHandle(0, &s.nvmlDevice);
        }

        ScopedSkipDxgiLoadChecks skipDxgiLoadChecks {};
        HMODULE nvapi = NtdllProxy::LoadLibraryExW_Ldr(L"nvapi64.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (nvapi != nullptr)
        {
            auto queryInterface = reinterpret_cast<PFN_NvApi_QueryInterface>(
                KernelBaseProxy::GetProcAddress_()(nvapi, "nvapi_QueryInterface"));

            // Skip fakenvapi
            if (queryInterface != nullptr && queryInterface(0x21382138) == nullptr)
            {
                auto* init = GET_INTERFACE(NvAPI_Initialize, queryInterface);
                if (init != nullptr && init() == NVAPI_OK)
                {
                    auto* getLogicalGpus = GET_INTERFACE(NvAPI_SYS_GetLogicalGPUs, queryInterface);
                    auto* getLogicalGpuInfo = GET_INTERFACE(NvAPI_GPU_GetLogicalGpuInfo, queryInterface);

                    NV_LOGICAL_GPUS logicalGpus {};
                    logicalGpus.version = NV_LOGICAL_GPUS_VER;
                    if (getLogicalGpus != nullptr && getLogicalGpuInfo != nullptr &&
                        getLogicalGpus(&logicalGpus) == NVAPI_OK)
                    {
                        for (uint32_t i = 0; i < logicalGpus.gpuHandleCount; i++)
                        {
                            LUID luid {};
                            NV_LOGICAL_GPU_DATA data {};
                            data.version = NV_LOGICAL_GPU_DATA_VER;
                            data.pOSAdapterId = &luid;
                            if (getLogicalGpuInfo(logicalGpus.gpuHandleData[i].hLogicalGpu, &data) == NVAPI_OK &&
                                data.physicalGpuCount > 0)
                            {
                                if (s.nvapiGpu == nullptr || IsEqualLUID(luid, gpuInfo.luid))
                                    s.nvapiGpu = data.physicalGpuHandles[0];
                            }
                        }
                    }

                    if (s.nvapiGpu == nullptr)
                    {
                        auto* enumGpus = GET_INTERFACE(NvAPI_EnumPhysicalGPUs, queryInterface);
                        NvPhysicalGpuHandle handles[NVAPI_MAX_PHYSICAL_GPUS] {};
                        NvU32 count = 0;
                        if (enumGpus != nullptr && enumGpus(handles, &count) == NVAPI_OK && count > 0)
                            s.nvapiGpu = handles[0];
                    }

                    s.getAllClocks = GET_INTERFACE(NvAPI_GPU_GetAllClockFrequencies, queryInterface);
                    s.getThermal = GET_INTERFACE(NvAPI_GPU_GetThermalSettings, queryInterface);
                    s.getCurrentVoltage =
                        reinterpret_cast<PFN_NvAPI_GPU_GetCurrentVoltage>(queryInterface(0x465F9BCFu));
                    s.getThermalSensors =
                        reinterpret_cast<PFN_NvAPI_GPU_ThermalGetSensors>(queryInterface(0x65FE3AADu));

                    // A mask with a sensor the GPU doesn't have fails, so the sensors are asked one more at a time
                    if (s.getThermalSensors != nullptr && s.nvapiGpu != nullptr)
                    {
                        for (int bit = 0; bit < 32; bit++)
                        {
                            NvThermalSensors sensors {};
                            sensors.version = NvThermalSensorsVersion;
                            sensors.mask = s.thermalSensorMask | (1u << bit);

                            if (s.getThermalSensors(s.nvapiGpu, &sensors) != NVAPI_OK)
                                break;

                            s.thermalSensorMask = sensors.mask;
                        }

                        LOG_DEBUG("FPS overlay: NVAPI thermal sensor mask 0x{:X}", s.thermalSensorMask);
                    }
                }
            }
        }
    }

    if (s.nvmlDevice != nullptr)
    {
        unsigned int val = 0;
        if (s.getTemp != nullptr && s.getTemp(s.nvmlDevice, 0, &val) == 0 && val > 0 && val < 150)
            out.tempC = static_cast<double>(val);

        if (s.getPower != nullptr && s.getPower(s.nvmlDevice, &val) == 0 && val > 0)
            out.powerW = static_cast<double>(val) / 1000.0;

        if (s.getClock != nullptr && s.getClock(s.nvmlDevice, 0, &val) == 0 && val > 0)
            out.coreClockMhz = val;

        if (s.getClock != nullptr && s.getClock(s.nvmlDevice, 2, &val) == 0 && val > 0)
            out.memClockMhz = val;
    }

    if (s.nvapiGpu != nullptr)
    {
        if (!out.coreClockMhz.has_value() || !out.memClockMhz.has_value())
        {
            NV_GPU_CLOCK_FREQUENCIES freqs {};
            freqs.version = NV_GPU_CLOCK_FREQUENCIES_VER;
            freqs.ClockType = NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ;
            if (s.getAllClocks != nullptr && s.getAllClocks(s.nvapiGpu, &freqs) == NVAPI_OK)
            {
                if (!out.coreClockMhz.has_value() && freqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].bIsPresent)
                    out.coreClockMhz = freqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency / 1000;

                if (!out.memClockMhz.has_value() && freqs.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].bIsPresent)
                    out.memClockMhz = freqs.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].frequency / 1000;
            }
        }

        if (!out.tempC.has_value() && s.getThermal != nullptr)
        {
            NV_GPU_THERMAL_SETTINGS thermal {};
            thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
            if (s.getThermal(s.nvapiGpu, NVAPI_THERMAL_TARGET_ALL, &thermal) == NVAPI_OK && thermal.count > 0 &&
                thermal.sensor[0].currentTemp > 0)
            {
                out.tempC = static_cast<double>(thermal.sensor[0].currentTemp);
            }
        }

        if ((s.thermalSensorMask & (1u << NvHotspotSensor)) != 0)
        {
            NvThermalSensors sensors {};
            sensors.version = NvThermalSensorsVersion;
            sensors.mask = s.thermalSensorMask;

            if (s.getThermalSensors(s.nvapiGpu, &sensors) == NVAPI_OK)
            {
                const double hotspot = sensors.temperatures[NvHotspotSensor] / 256.0;

                if (hotspot > 0.0 && hotspot < 150.0)
                {
                    out.hotspotC = hotspot;
                    LogSource("GPU hot spot", "NVAPI");
                }
            }
        }

        if (!out.voltageV.has_value() && s.getCurrentVoltage != nullptr)
        {
            NvGpuVoltageStatus volt {};
            volt.version = static_cast<NvU32>((72u << 16) | 1u);
            if (s.getCurrentVoltage(s.nvapiGpu, &volt) == NVAPI_OK && volt.value_uV > 0)
            {
                const double v = volt.value_uV > 10000 ? volt.value_uV / 1'000'000.0 : volt.value_uV / 1000.0;
                if (v >= 0.3 && v <= 2.0)
                    out.voltageV = v;
            }
        }
    }
}
} // namespace Nvidia

// Shared Memory readers for HWiNFO64, MSI Afterburner, CoreTemp, and AMD Adrenalin CPUMetricsServer
namespace SharedSensors
{
#pragma pack(push, 1)
struct HWiNFOHeader
{
    uint32_t signature;
    uint32_t version;
    uint32_t revision;
    int64_t pollTime;
    uint32_t sensorOffset;
    uint32_t sensorElementSize;
    uint32_t sensorElementCount;
    uint32_t readingOffset;
    uint32_t readingElementSize;
    uint32_t readingElementCount;
};

struct HWiNFOReadingPrefix
{
    uint32_t readingType; // 1=Temp, 2=Volt, 3=Fan, 4=Current, 5=Power, 6=Clock, 7=Usage
    uint32_t sensorIndex;
    uint32_t readingId;
    char labelOrig[128];
    char labelUser[128];
    char unit[16];
    double value;
};

struct HWiNFOSensorPrefix
{
    uint32_t sensorId;
    uint32_t sensorInstance;
    char nameOrig[128];
    char nameUser[128];
};

struct MAHMHeader
{
    uint32_t signature;
    uint32_t version;
    uint32_t headerSize;
    uint32_t entryCount;
    uint32_t entrySize;
    int32_t time;
};

struct MAHMEntry
{
    char srcName[260];
    char srcUnits[260];
    char localizedSrcName[260];
    char localizedSrcUnits[260];
    char recommendedFormat[260];
    float data;
    float minLimit;
    float maxLimit;
    uint32_t flags;
    uint32_t gpu;
    uint32_t srcId;
};

struct CoreTempData
{
    uint32_t load[256];
    uint32_t tjMax[128];
    uint32_t coreCount;
    uint32_t cpuCount;
    float temp[256];
    float vid;
    float cpuSpeed;
    float fsbSpeed;
    float multiplier;
    char cpuName[100];
    uint8_t fahrenheit;
    uint8_t deltaToTjMax;
    uint8_t tdpSupported;
    uint8_t powerSupported;
    uint32_t structVersion;
    uint32_t tdp[128];
    float power[128];
};
#pragma pack(pop)

inline bool ContainsCaseInsensitive(const char* haystack, const char* needle)
{
    if (haystack == nullptr || needle == nullptr || *needle == '\0')
        return false;

    const size_t nLen = std::strlen(needle);
    for (const char* p = haystack; *p != '\0'; ++p)
    {
        if (_strnicmp(p, needle, nLen) == 0)
            return true;
    }
    return false;
}

// AMD Adrenalin CPU Metrics Server ("954b280e-5529-4492-bab2-bb7a502552f8")
inline void QueryAmdCpuServer(CpuMetrics& out)
{
    static constexpr const wchar_t* kMapName = L"954b280e-5529-4492-bab2-bb7a502552f8";
    static constexpr const wchar_t* kEventName = L"954b280e-5529-4492-bab2-bb7a502552f8_event";
    static constexpr size_t kMapSize = 0x1F8;

    static HANDLE hMap = nullptr;
    static uint8_t* base = nullptr;
    static HANDLE hEvent = nullptr;
    static bool taskLaunchAttempted = false;
    static uint64_t lastRetryTick = 0;

    if (base == nullptr)
    {
        const uint64_t nowTick = GetTickCount64();
        if (lastRetryTick != 0 && nowTick - lastRetryTick < 2000)
            return;

        lastRetryTick = nowTick;

        hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kMapName);
        if (hMap == nullptr && !taskLaunchAttempted)
        {
            taskLaunchAttempted = true;
            if (std::filesystem::exists(L"C:\\Program Files\\AMD\\CNext\\CNext\\cpumetricsserver.exe"))
            {
                wchar_t cmdLine[] = L"schtasks.exe /run /TN \"AMDRyzenMasterSDKTask\"";
                STARTUPINFOW si {};
                si.cb = sizeof(si);
                PROCESS_INFORMATION pi {};
                if (CreateProcessW(nullptr, cmdLine, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si,
                                   &pi))
                {
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
            }
        }

        if (hMap != nullptr)
        {
            base = static_cast<uint8_t*>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, kMapSize));
            if (base == nullptr)
            {
                CloseHandle(hMap);
                hMap = nullptr;
            }
            else
            {
                hEvent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, kEventName);
                base[0] = 1;
                if (hEvent != nullptr)
                    SetEvent(hEvent);
            }
        }
    }

    if (base == nullptr)
        return;

    // Descriptor support flags at +0x38 (clock), +0x58 (volt), +0x78 (temp), +0x98 (power)
    const uint16_t suppClock = *reinterpret_cast<const uint16_t*>(base + 0x38);
    const uint16_t suppVolt = *reinterpret_cast<const uint16_t*>(base + 0x58);
    const uint16_t suppTemp = *reinterpret_cast<const uint16_t*>(base + 0x78);
    const uint16_t suppPower = *reinterpret_cast<const uint16_t*>(base + 0x98);

    const double clk = *reinterpret_cast<const double*>(base + 0x1C8);
    const double volt = *reinterpret_cast<const double*>(base + 0x1D0);
    const double temp = *reinterpret_cast<const double*>(base + 0x1D8);
    const float power = *reinterpret_cast<const float*>(base + 0x1E0);

    if (suppClock != 0 && clk > 100.0 && clk < 10000.0)
        out.clockMhz = static_cast<uint32_t>(std::lround(clk));

    if (suppVolt != 0 && volt >= 0.2 && volt <= 2.5)
        out.voltageV = volt;

    if (suppTemp != 0 && temp > 0.0 && temp < 150.0)
    {
        out.tempC = temp;
        LogSource("CPU temperature", "AMD Adrenalin");
    }

    if (suppPower != 0 && power > 0.1f && power < 1000.0f)
        out.powerW = static_cast<double>(power);

    // Request next sample asynchronously for the next poll
    base[0] = 1;
    if (hEvent != nullptr)
        SetEvent(hEvent);
}

// The hottest of a CPU's readings, its cores, CCDs or AMD's Tctl
inline void KeepHottest(std::optional<double>& hottest, double value)
{
    if (value > 0.0 && value < 150.0 && (!hottest.has_value() || value > hottest.value()))
        hottest = value;
}

// HWiNFO64 with Shared Memory Support enabled in its settings (the free version turns it off after 12 hours).
// The GPU's readings are taken from the sensor named as the GPU, when there's one, so an iGPU's aren't.
inline void QueryHwInfo(GpuMetrics* gpu, CpuMetrics* cpu, const std::string& gpuName = {})
{
    static uint64_t lastMissTick = 0;
    const uint64_t nowTick = GetTickCount64();
    if (lastMissTick != 0 && nowTick - lastMissTick < 5000)
        return;

    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "Global\\HWiNFO_SENS_SM2");
    if (hMap == nullptr)
        hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "HWiNFO_SENS_SM2");

    if (hMap == nullptr)
    {
        lastMissTick = nowTick;
        return;
    }

    lastMissTick = 0;

    const auto* base = static_cast<const uint8_t*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));
    if (base != nullptr)
    {
        const auto* hdr = reinterpret_cast<const HWiNFOHeader*>(base);

        // 'DEAD' when HWiNFO closed or its shared memory got turned off
        if (hdr->signature == 0x53695748u && hdr->readingElementSize >= sizeof(HWiNFOReadingPrefix) &&
            hdr->readingElementCount <= 16384 && hdr->sensorElementSize >= sizeof(HWiNFOSensorPrefix) &&
            hdr->sensorElementCount <= 1024)
        {
            auto sensorName = [&](uint32_t index) -> const char*
            {
                if (index >= hdr->sensorElementCount)
                    return "";

                return reinterpret_cast<const HWiNFOSensorPrefix*>(base + hdr->sensorOffset +
                                                                   index * hdr->sensorElementSize)
                    ->nameOrig;
            };

            bool gpuSensorFound = false;
            if (gpu != nullptr && !gpuName.empty())
            {
                for (uint32_t i = 0; i < hdr->sensorElementCount && !gpuSensorFound; ++i)
                    gpuSensorFound = ContainsCaseInsensitive(sensorName(i), gpuName.c_str());
            }

            bool cpuFound = false;
            bool gpuFound = false;

            for (uint32_t i = 0; i < hdr->readingElementCount; ++i)
            {
                const auto* r = reinterpret_cast<const HWiNFOReadingPrefix*>(base + hdr->readingOffset +
                                                                             i * hdr->readingElementSize);
                if (!std::isfinite(r->value) || r->value <= 0.0)
                    continue;

                if (cpu != nullptr)
                {
                    const bool temp = r->readingType == 1 && !ContainsCaseInsensitive(r->labelOrig, "Distance");

                    // AMD's Tctl/Tdie and CCDs, Intel's package and hottest core
                    if (temp && (ContainsCaseInsensitive(r->labelOrig, "CPU (Tctl") ||
                                 ContainsCaseInsensitive(r->labelOrig, "CCD") ||
                                 ContainsCaseInsensitive(r->labelOrig, "Core Max")))
                    {
                        KeepHottest(cpu->hotspotC, r->value);
                        cpuFound = true;
                    }

                    if (temp && !cpu->tempC.has_value() &&
                        (ContainsCaseInsensitive(r->labelOrig, "CPU (Tctl/Tdie)") ||
                         ContainsCaseInsensitive(r->labelOrig, "CPU Die") ||
                         ContainsCaseInsensitive(r->labelOrig, "CPU Package")))
                    {
                        cpu->tempC = r->value;
                        cpuFound = true;
                    }
                    else if (r->readingType == 5 && !cpu->powerW.has_value() &&
                             (ContainsCaseInsensitive(r->labelOrig, "CPU Package Power") ||
                              ContainsCaseInsensitive(r->labelOrig, "CPU PPT")))
                    {
                        cpu->powerW = r->value;
                    }
                    else if (r->readingType == 2 && !cpu->voltageV.has_value() &&
                             (ContainsCaseInsensitive(r->labelOrig, "CPU Core Voltage") ||
                              ContainsCaseInsensitive(r->labelOrig, "Vcore") ||
                              ContainsCaseInsensitive(r->labelOrig, "CPU Core VID")))
                    {
                        cpu->voltageV = r->value;
                    }
                    else if (r->readingType == 6 && !cpu->clockMhz.has_value() &&
                             (ContainsCaseInsensitive(r->labelOrig, "Core Clocks") ||
                              ContainsCaseInsensitive(r->labelOrig, "Core 0 Clock") ||
                              ContainsCaseInsensitive(r->labelOrig, "Core #0 Clock")))
                    {
                        cpu->clockMhz = static_cast<uint32_t>(std::lround(r->value));
                    }
                }

                if (gpu != nullptr &&
                    (!gpuSensorFound || ContainsCaseInsensitive(sensorName(r->sensorIndex), gpuName.c_str())))
                {
                    if (r->readingType == 1 && !gpu->hotspotC.has_value() &&
                        ContainsCaseInsensitive(r->labelOrig, "GPU") &&
                        (ContainsCaseInsensitive(r->labelOrig, "Hot Spot") ||
                         ContainsCaseInsensitive(r->labelOrig, "Hotspot")))
                    {
                        gpu->hotspotC = r->value;
                        gpuFound = true;
                    }
                    else if (r->readingType == 1 && !gpu->tempC.has_value() &&
                             (ContainsCaseInsensitive(r->labelOrig, "GPU Temperature") ||
                              ContainsCaseInsensitive(r->labelOrig, "GPU Temp")))
                    {
                        gpu->tempC = r->value;
                        gpuFound = true;
                    }
                    else if (r->readingType == 5 && !gpu->powerW.has_value() &&
                             (ContainsCaseInsensitive(r->labelOrig, "GPU Power") ||
                              ContainsCaseInsensitive(r->labelOrig, "Total Board Power") ||
                              ContainsCaseInsensitive(r->labelOrig, "GPU ASIC Power")))
                    {
                        gpu->powerW = r->value;
                    }
                    else if (r->readingType == 2 && !gpu->voltageV.has_value() &&
                             (ContainsCaseInsensitive(r->labelOrig, "GPU Core Voltage") ||
                              ContainsCaseInsensitive(r->labelOrig, "GPU Voltage")))
                    {
                        gpu->voltageV = r->value > 10.0 ? r->value / 1000.0 : r->value;
                    }
                    else if (r->readingType == 6 && !gpu->coreClockMhz.has_value() &&
                             (ContainsCaseInsensitive(r->labelOrig, "GPU Clock") ||
                              ContainsCaseInsensitive(r->labelOrig, "GPU Core Clock")))
                    {
                        gpu->coreClockMhz = static_cast<uint32_t>(std::lround(r->value));
                    }
                    else if (r->readingType == 6 && !gpu->memClockMhz.has_value() &&
                             ContainsCaseInsensitive(r->labelOrig, "GPU Memory Clock"))
                    {
                        gpu->memClockMhz = static_cast<uint32_t>(std::lround(r->value));
                    }
                }
            }

            if (cpuFound)
                LogSource("CPU temperature", "HWiNFO");

            if (gpuFound)
                LogSource("GPU temperature", "HWiNFO");
        }
        UnmapViewOfFile(base);
    }

    CloseHandle(hMap);
}

// MSI Afterburner's hardware monitoring, its CPU readings need them shown in its monitoring settings
inline void QueryAfterburner(GpuMetrics* gpu, CpuMetrics* cpu)
{
    // MONITORING_SOURCE_ID_* of the MSI Afterburner SDK
    constexpr uint32_t srcGpuTemperature = 0x00;
    constexpr uint32_t srcCoreClock = 0x20;
    constexpr uint32_t srcMemoryClock = 0x22;
    constexpr uint32_t srcGpuVoltage = 0x40;
    constexpr uint32_t srcGpuPower = 0x60;
    constexpr uint32_t srcCpuTemperature = 0x80;
    constexpr uint32_t srcCpuClock = 0xA0;
    constexpr uint32_t srcCpuPower = 0x100;

    static uint64_t lastMissTick = 0;
    const uint64_t nowTick = GetTickCount64();
    if (lastMissTick != 0 && nowTick - lastMissTick < 5000)
        return;

    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "MAHMSharedMemory");
    if (hMap == nullptr)
    {
        lastMissTick = nowTick;
        return;
    }

    lastMissTick = 0;

    const auto* base = static_cast<const uint8_t*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));
    if (base != nullptr)
    {
        const auto* hdr = reinterpret_cast<const MAHMHeader*>(base);

        // 'MAHM' as the SDK writes it, 0xDEAD once Afterburner closed
        if (hdr->signature == 0x4D41484Du && hdr->entrySize >= sizeof(MAHMEntry) && hdr->entryCount <= 512)
        {
            bool cpuFound = false;
            bool gpuFound = false;

            for (uint32_t i = 0; i < hdr->entryCount; ++i)
            {
                const auto* e = reinterpret_cast<const MAHMEntry*>(base + hdr->headerSize + i * hdr->entrySize);
                if (!std::isfinite(e->data) || e->data <= 0.0f || e->data >= 1.0e10f)
                    continue;

                if (gpu != nullptr && (e->gpu == 0 || e->gpu == 0xFFFFFFFFu))
                {
                    // Not a source of its own, the hot spot is told by its name
                    if (!gpu->hotspotC.has_value() && (ContainsCaseInsensitive(e->srcName, "hot spot") ||
                                                       ContainsCaseInsensitive(e->srcName, "hotspot")))
                    {
                        gpu->hotspotC = e->data;
                        gpuFound = true;
                    }
                    else if (e->srcId == srcGpuTemperature && !gpu->tempC.has_value())
                    {
                        gpu->tempC = e->data;
                        gpuFound = true;
                    }
                    else if (e->srcId == srcGpuPower && !gpu->powerW.has_value() &&
                             _stricmp(e->srcUnits, "W") == 0) // Else it's percents of the power limit
                    {
                        gpu->powerW = e->data;
                    }
                    else if (e->srcId == srcGpuVoltage && !gpu->voltageV.has_value())
                    {
                        gpu->voltageV = e->data > 10.0f ? e->data / 1000.0 : e->data;
                    }
                    else if (e->srcId == srcCoreClock && !gpu->coreClockMhz.has_value())
                    {
                        gpu->coreClockMhz = static_cast<uint32_t>(std::lround(e->data));
                    }
                    else if (e->srcId == srcMemoryClock && !gpu->memClockMhz.has_value())
                    {
                        gpu->memClockMhz = static_cast<uint32_t>(std::lround(e->data));
                    }
                }

                if (cpu != nullptr)
                {
                    // "CPU temperature" and one per core
                    if (e->srcId == srcCpuTemperature && e->gpu == 0xFFFFFFFFu)
                    {
                        KeepHottest(cpu->hotspotC, e->data);

                        if (!cpu->tempC.has_value() && _stricmp(e->srcName, "CPU temperature") == 0)
                            cpu->tempC = e->data;

                        cpuFound = true;
                    }
                    else if (e->srcId == srcCpuPower && !cpu->powerW.has_value())
                    {
                        cpu->powerW = e->data;
                    }
                    else if (e->srcId == srcCpuClock && e->gpu == 0xFFFFFFFFu && !cpu->clockMhz.has_value())
                    {
                        cpu->clockMhz = static_cast<uint32_t>(std::lround(e->data));
                    }
                }
            }

            // Only the cores' temperatures shown
            if (cpu != nullptr && !cpu->tempC.has_value() && cpu->hotspotC.has_value())
                cpu->tempC = cpu->hotspotC;

            if (cpuFound)
                LogSource("CPU temperature", "MSI Afterburner");

            if (gpuFound)
                LogSource("GPU temperature", "MSI Afterburner");
        }
        UnmapViewOfFile(base);
    }

    CloseHandle(hMap);
}

inline void QueryCoreTemp(CpuMetrics& cpu)
{
    static uint64_t lastMissTick = 0;
    const uint64_t nowTick = GetTickCount64();
    if (lastMissTick != 0 && nowTick - lastMissTick < 5000)
        return;

    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "CoreTempMappingObjectEx");
    if (hMap == nullptr)
    {
        lastMissTick = nowTick;
        return;
    }

    lastMissTick = 0;

    const auto* data = static_cast<const CoreTempData*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(CoreTempData)));
    if (data != nullptr)
    {
        // Core Temp has the cores only, the hottest is also the CPU's temperature
        if ((!cpu.tempC.has_value() || !cpu.hotspotC.has_value()) && data->coreCount > 0 && data->coreCount <= 256)
        {
            float maxTemp = 0.0f;
            for (uint32_t i = 0; i < data->coreCount; ++i)
            {
                float t = data->temp[i];
                if (data->deltaToTjMax != 0 && data->tjMax[0] > 0)
                    t = static_cast<float>(data->tjMax[0]) - t;
                if (data->fahrenheit != 0)
                    t = (t - 32.0f) * (5.0f / 9.0f);
                maxTemp = std::max(maxTemp, t);
            }
            if (maxTemp > 0.0f && maxTemp < 150.0f)
            {
                if (!cpu.tempC.has_value())
                    cpu.tempC = maxTemp;

                if (!cpu.hotspotC.has_value())
                    cpu.hotspotC = maxTemp;

                LogSource("CPU temperature", "Core Temp");
            }
        }

        if (!cpu.voltageV.has_value() && data->vid >= 0.2f && data->vid <= 2.5f)
            cpu.voltageV = data->vid;

        if (!cpu.clockMhz.has_value() && data->cpuSpeed > 100.0f && data->cpuSpeed < 10000.0f)
            cpu.clockMhz = static_cast<uint32_t>(std::lround(data->cpuSpeed));

        if (!cpu.powerW.has_value() && data->powerSupported != 0 && data->power[0] > 0.1f && data->power[0] < 1000.0f)
            cpu.powerW = data->power[0];

        UnmapViewOfFile(data);
    }

    CloseHandle(hMap);
}
} // namespace SharedSensors

// Linux sensors (hwmon in /sys) read through Wine's path to the host's files, under Proton there's no Windows driver
// that gives temperatures. k10temp and zenpower (AMD CPUs), coretemp (Intel CPUs), amdgpu.
namespace Hwmon
{
using PFN_wine_get_dos_file_name = LPWSTR(CDECL*)(LPCSTR);

// A host path as a Windows one, empty outside Wine
inline std::wstring DosPath(const std::string& unixPath)
{
    static auto toDos = []() -> PFN_wine_get_dos_file_name
    {
        if (!State::Instance().isRunningOnLinux)
            return nullptr;

        HMODULE kernel32 = KernelBaseProxy::GetModuleHandleW_()(L"kernel32.dll");
        return kernel32 != nullptr ? reinterpret_cast<PFN_wine_get_dos_file_name>(
                                         KernelBaseProxy::GetProcAddress_()(kernel32, "wine_get_dos_file_name"))
                                   : nullptr;
    }();

    if (toDos == nullptr)
        return {};

    LPWSTR dos = toDos(unixPath.c_str());
    if (dos == nullptr)
        return {};

    std::wstring path(dos);
    HeapFree(GetProcessHeap(), 0, dos);
    return path;
}

// A sysfs file's first line, sysfs files say they're 4096 bytes long so it reads what there is
inline std::optional<std::string> ReadLine(const std::wstring& path)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return std::nullopt;

    char buffer[128] {};
    DWORD read = 0;
    const BOOL ok = ReadFile(file, buffer, sizeof(buffer) - 1, &read, nullptr);
    CloseHandle(file);

    if (!ok || read == 0)
        return std::nullopt;

    std::string line(buffer, read);
    if (auto end = line.find_first_of("\r\n"); end != std::string::npos)
        line.resize(end);

    return line;
}

inline std::optional<double> ReadNumber(const std::wstring& path)
{
    auto line = ReadLine(path);
    if (!line.has_value() || line->empty())
        return std::nullopt;

    char* end = nullptr;
    const double value = std::strtod(line->c_str(), &end);
    if (end == line->c_str() || !std::isfinite(value))
        return std::nullopt;

    return value;
}

struct Device
{
    std::string name;  // The driver: k10temp, coretemp, amdgpu...
    std::wstring path; // Its folder, ending in a separator
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
};

// The hwmon devices, listed once
inline const std::vector<Device>& Devices()
{
    static const std::vector<Device> devices = []()
    {
        std::vector<Device> found;
        std::wstring root = DosPath("/sys/class/hwmon");
        if (root.empty())
            return found;

        if (root.back() != L'\\')
            root += L'\\';

        WIN32_FIND_DATAW data {};
        HANDLE find = FindFirstFileW((root + L"hwmon*").c_str(), &data);
        if (find == INVALID_HANDLE_VALUE)
            return found;

        do
        {
            Device device;
            device.path = root + data.cFileName + L"\\";

            if (auto name = ReadLine(device.path + L"name"); name.has_value())
                device.name = name.value();

            // PCI devices, the GPUs
            if (auto vendor = ReadLine(device.path + L"device\\vendor"); vendor.has_value())
                device.vendorId = static_cast<uint32_t>(std::strtoul(vendor->c_str(), nullptr, 16));

            if (auto id = ReadLine(device.path + L"device\\device"); id.has_value())
                device.deviceId = static_cast<uint32_t>(std::strtoul(id->c_str(), nullptr, 16));

            LOG_DEBUG("FPS overlay: hwmon {} {:04X}:{:04X}", device.name, device.vendorId, device.deviceId);
            found.push_back(std::move(device));
        } while (FindNextFileW(find, &data));

        FindClose(find);
        return found;
    }();

    return devices;
}

// A device's temperatures in C by their label, "tempN" when they have none
inline std::vector<std::pair<std::string, double>> Temperatures(const Device& device)
{
    std::vector<std::pair<std::string, double>> temps;

    for (int i = 1; i <= 32; i++)
    {
        const std::wstring prefix = device.path + L"temp" + std::to_wstring(i);
        auto milli = ReadNumber(prefix + L"_input");
        if (!milli.has_value())
            continue;

        const double celsius = milli.value() / 1000.0;
        if (celsius <= 0.0 || celsius >= 150.0)
            continue;

        temps.emplace_back(ReadLine(prefix + L"_label").value_or("temp" + std::to_string(i)), celsius);
    }

    return temps;
}

inline void QueryCpu(CpuMetrics& out)
{
    for (const auto& device : Devices())
    {
        const bool amd = device.name == "k10temp" || device.name == "zenpower";
        if (!amd && device.name != "coretemp")
            continue;

        for (const auto& [label, celsius] : Temperatures(device))
        {
            // AMD: Tctl (or zenpower's Tdie) and the CCDs, Intel: the package and the cores
            const bool package = amd ? (label == "Tctl" || label == "Tdie") : label.starts_with("Package");
            const bool core = amd ? label.starts_with("Tccd") || label == "Tctl" : label.starts_with("Core");

            if (package && (!out.tempC.has_value() || label == "Tctl"))
                out.tempC = celsius;

            if (core || package)
                SharedSensors::KeepHottest(out.hotspotC, celsius);
        }

        if (out.tempC.has_value())
        {
            LogSource("CPU temperature", ("hwmon " + device.name).c_str());
            return;
        }
    }
}

// amdgpu's readings for the GPU with that PCI id
inline void QueryGpu(const GpuInformation& gpuInfo, GpuMetrics& out)
{
    for (const auto& device : Devices())
    {
        if (device.name != "amdgpu" || device.vendorId != 0x1002 ||
            (gpuInfo.deviceId != 0 && device.deviceId != gpuInfo.deviceId))
        {
            continue;
        }

        for (const auto& [label, celsius] : Temperatures(device))
        {
            if (label == "edge" && !out.tempC.has_value())
                out.tempC = celsius;
            else if (label == "junction" && !out.hotspotC.has_value())
                out.hotspotC = celsius;
        }

        // Only the hot spot on some APUs
        if (!out.tempC.has_value() && out.hotspotC.has_value())
            out.tempC = out.hotspotC;

        // Micro watts, the average on older kernels
        if (!out.powerW.has_value())
        {
            auto power = ReadNumber(device.path + L"power1_average");
            if (!power.has_value() || power.value() <= 0.0)
                power = ReadNumber(device.path + L"power1_input");

            if (power.has_value() && power.value() > 0.0)
                out.powerW = power.value() / 1'000'000.0;
        }

        // Millivolts
        if (auto mv = ReadNumber(device.path + L"in0_input"); !out.voltageV.has_value() && mv && *mv > 0.0)
            out.voltageV = *mv / 1000.0;

        // Hertz, sclk and mclk
        if (auto hz = ReadNumber(device.path + L"freq1_input"); !out.coreClockMhz.has_value() && hz && *hz > 0.0)
            out.coreClockMhz = static_cast<uint32_t>(std::lround(*hz / 1'000'000.0));

        if (auto hz = ReadNumber(device.path + L"freq2_input"); !out.memClockMhz.has_value() && hz && *hz > 0.0)
            out.memClockMhz = static_cast<uint32_t>(std::lround(*hz / 1'000'000.0));

        if (out.tempC.has_value())
            LogSource("GPU temperature", "hwmon amdgpu");

        return;
    }
}
} // namespace Hwmon
// Windows PDH effective CPU frequency (universal fallback for CPU clock MHz)
namespace PdhCpu
{
struct PdhCounterValue
{
    DWORD CStatus;
    LONG padding;
    double doubleValue;
};

using PFN_PdhOpenQueryW = LONG(__stdcall*)(LPCWSTR, DWORD_PTR, HANDLE*);
using PFN_PdhAddEnglishCounterW = LONG(__stdcall*)(HANDLE, LPCWSTR, DWORD_PTR, HANDLE*);
using PFN_PdhCollectQueryData = LONG(__stdcall*)(HANDLE);
using PFN_PdhGetFormattedCounterValue = LONG(__stdcall*)(HANDLE, DWORD, LPDWORD, PdhCounterValue*);

inline std::optional<uint32_t> QueryClockMhz()
{
    static bool initialized = false;
    static HANDLE query = nullptr;
    static HANDLE cFreq = nullptr;
    static HANDLE cPerf = nullptr;
    static PFN_PdhCollectQueryData collect = nullptr;
    static PFN_PdhGetFormattedCounterValue getFormatted = nullptr;

    if (!initialized)
    {
        initialized = true;
        HMODULE pdh = KernelBaseProxy::GetModuleHandleW_()(L"pdh.dll");
        if (pdh == nullptr)
            pdh = NtdllProxy::LoadLibraryExW_Ldr(L"pdh.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

        if (pdh != nullptr)
        {
            auto openQuery =
                reinterpret_cast<PFN_PdhOpenQueryW>(KernelBaseProxy::GetProcAddress_()(pdh, "PdhOpenQueryW"));
            auto addCounter = reinterpret_cast<PFN_PdhAddEnglishCounterW>(
                KernelBaseProxy::GetProcAddress_()(pdh, "PdhAddEnglishCounterW"));
            collect = reinterpret_cast<PFN_PdhCollectQueryData>(
                KernelBaseProxy::GetProcAddress_()(pdh, "PdhCollectQueryData"));
            getFormatted = reinterpret_cast<PFN_PdhGetFormattedCounterValue>(
                KernelBaseProxy::GetProcAddress_()(pdh, "PdhGetFormattedCounterValue"));

            if (openQuery != nullptr && addCounter != nullptr && collect != nullptr && getFormatted != nullptr &&
                openQuery(nullptr, 0, &query) == 0)
            {
                addCounter(query, L"\\Processor Information(_Total)\\Processor Frequency", 0, &cFreq);
                addCounter(query, L"\\Processor Information(_Total)\\% Processor Performance", 0, &cPerf);
                collect(query);
            }
        }
    }

    if (query == nullptr || collect == nullptr || getFormatted == nullptr || collect(query) != 0)
        return std::nullopt;

    DWORD type = 0;
    PdhCounterValue vFreq {};
    PdhCounterValue vPerf {};
    if (cFreq != nullptr && getFormatted(cFreq, 0x00000200 /*PDH_FMT_DOUBLE*/, &type, &vFreq) == 0 &&
        vFreq.doubleValue > 100.0)
    {
        double mhz = vFreq.doubleValue;
        if (cPerf != nullptr && getFormatted(cPerf, 0x00000200 /*PDH_FMT_DOUBLE*/, &type, &vPerf) == 0 &&
            vPerf.doubleValue > 1.0)
        {
            mhz = mhz * (vPerf.doubleValue / 100.0);
        }
        return static_cast<uint32_t>(std::lround(mhz));
    }

    return std::nullopt;
}

// All the CPU's time not spent idle since the last call, the first call has nothing to compare to
inline std::optional<double> QueryUsagePercent()
{
    static uint64_t lastIdle = 0;
    static uint64_t lastTotal = 0;

    FILETIME idleTime {};
    FILETIME kernelTime {};
    FILETIME userTime {};
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return std::nullopt;

    auto toU64 = [](const FILETIME& time)
    { return (static_cast<uint64_t>(time.dwHighDateTime) << 32) | time.dwLowDateTime; };

    // Kernel time includes the idle time
    const uint64_t idle = toU64(idleTime);
    const uint64_t total = toU64(kernelTime) + toU64(userTime);

    std::optional<double> usage;
    if (lastTotal != 0 && total > lastTotal && idle >= lastIdle)
    {
        const double busy = 1.0 - static_cast<double>(idle - lastIdle) / static_cast<double>(total - lastTotal);
        usage = std::clamp(busy * 100.0, 0.0, 100.0);
    }

    lastIdle = idle;
    lastTotal = total;
    return usage;
}
} // namespace PdhCpu

// The GPU's usage as Task Manager shows it: the "GPU Engine" counters of the adapter's engines summed over the
// processes, the busiest engine is the usage. Any vendor.
namespace PdhGpu
{
struct PdhCounterItem
{
    LPWSTR name;
    PdhCpu::PdhCounterValue value;
};

using PFN_PdhGetFormattedCounterArrayW = LONG(__stdcall*)(HANDLE, DWORD, LPDWORD, LPDWORD, PdhCounterItem*);

inline std::optional<double> QueryUsagePercent(LUID luid)
{
    static bool initialized = false;
    static HANDLE query = nullptr;
    static HANDLE counter = nullptr;
    static PdhCpu::PFN_PdhCollectQueryData collect = nullptr;
    static PFN_PdhGetFormattedCounterArrayW getArray = nullptr;
    static std::vector<uint8_t> buffer;

    if (!initialized)
    {
        initialized = true;

        // The counters' provider loads its own modules on the first collect
        ScopedSkipDxgiLoadChecks skipDxgiLoadChecks {};
        HMODULE pdh = KernelBaseProxy::GetModuleHandleW_()(L"pdh.dll");
        if (pdh == nullptr)
            pdh = NtdllProxy::LoadLibraryExW_Ldr(L"pdh.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

        if (pdh != nullptr)
        {
            auto openQuery =
                reinterpret_cast<PdhCpu::PFN_PdhOpenQueryW>(KernelBaseProxy::GetProcAddress_()(pdh, "PdhOpenQueryW"));
            auto addCounter = reinterpret_cast<PdhCpu::PFN_PdhAddEnglishCounterW>(
                KernelBaseProxy::GetProcAddress_()(pdh, "PdhAddEnglishCounterW"));
            collect = reinterpret_cast<PdhCpu::PFN_PdhCollectQueryData>(
                KernelBaseProxy::GetProcAddress_()(pdh, "PdhCollectQueryData"));
            getArray = reinterpret_cast<PFN_PdhGetFormattedCounterArrayW>(
                KernelBaseProxy::GetProcAddress_()(pdh, "PdhGetFormattedCounterArrayW"));

            if (openQuery != nullptr && addCounter != nullptr && collect != nullptr && getArray != nullptr &&
                openQuery(nullptr, 0, &query) == 0)
            {
                if (addCounter(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) == 0)
                    collect(query);
                else
                    counter = nullptr;
            }
        }
    }

    if (query == nullptr || counter == nullptr || collect(query) != 0)
        return std::nullopt;

    constexpr DWORD format = 0x00000200 /*PDH_FMT_DOUBLE*/ | 0x00008000 /*PDH_FMT_NOCAP100*/;
    constexpr LONG moreData = static_cast<LONG>(0x800007D2) /*PDH_MORE_DATA*/;

    DWORD size = static_cast<DWORD>(buffer.size());
    DWORD count = 0;
    LONG result = getArray(counter, format, &size, &count,
                           buffer.empty() ? nullptr : reinterpret_cast<PdhCounterItem*>(buffer.data()));

    if (result == moreData)
    {
        buffer.resize(size);
        result = getArray(counter, format, &size, &count, reinterpret_cast<PdhCounterItem*>(buffer.data()));
    }

    if (result != 0 || count == 0)
        return std::nullopt;

    // Instances are pid_<pid>_luid_0x<high>_0x<low>_phys_<n>_eng_<n>_engtype_<type>
    wchar_t luidTag[40] {};
    swprintf_s(luidTag, L"luid_0x%08X_0x%08X_", static_cast<uint32_t>(luid.HighPart), luid.LowPart);

    std::vector<std::pair<std::wstring_view, double>> engines;
    const auto* items = reinterpret_cast<const PdhCounterItem*>(buffer.data());

    for (DWORD i = 0; i < count; i++)
    {
        if (items[i].name == nullptr || items[i].value.CStatus > 1 /*PDH_CSTATUS_VALID_DATA, NEW_DATA*/)
            continue;

        const std::wstring_view name = items[i].name;
        const auto luidAt = name.find(luidTag);
        if (luidAt == std::wstring_view::npos)
            continue;

        const auto engine = name.substr(luidAt);
        auto it = std::find_if(engines.begin(), engines.end(), [&](const auto& e) { return e.first == engine; });

        if (it == engines.end())
            engines.emplace_back(engine, items[i].value.doubleValue);
        else
            it->second += items[i].value.doubleValue;
    }

    if (engines.empty())
        return std::nullopt;

    double busiest = 0.0;
    for (const auto& engine : engines)
        busiest = std::max(busiest, engine.second);

    return std::clamp(busiest, 0.0, 100.0);
}
} // namespace PdhGpu

// Video memory the game uses and the budget Windows gives it, in bytes
inline std::optional<std::pair<uint64_t, uint64_t>> PollVram(const GpuInformation& gpuInfo)
{
    static IDXGIAdapter3* adapter = nullptr;
    static LUID adapterLuid {};

    const LUID luid = gpuInfo.luid;

    if (adapter == nullptr || !IsEqualLUID(luid, adapterLuid))
    {
        if (adapter != nullptr)
            adapter->Release();

        adapter = nullptr;
        adapterLuid = luid;

        ScopedSkipSpoofingThread skipSpoofingThread {};
        DxgiProxy::Init();

        IDXGIFactory4* factory = nullptr;
        HRESULT result = E_FAIL;

        if (DxgiProxy::CreateDxgiFactory_() != nullptr)
        {
            ScopedCreatingD3DDevice creatingD3DDevice {};
            result = DxgiProxy::CreateDxgiFactory_()(__uuidof(factory), (IDXGIFactory**) &factory);
        }

        if (result == S_OK && factory != nullptr)
        {
            if (factory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&adapter)) != S_OK)
                adapter = nullptr;

            factory->Release();
        }
    }

    DXGI_QUERY_VIDEO_MEMORY_INFO info {};
    if (adapter == nullptr || adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info) != S_OK)
        return std::nullopt;

    return std::make_pair(info.CurrentUsage, info.Budget);
}

inline GpuMetrics PollGpu(const GpuInformation& gpuInfo)
{
    GpuMetrics out {};

    if (gpuInfo.vendorId == VendorId::AMD)
    {
        Adl::ADLPMLogDataOutput pmLog {};
        if (Adl::PollPmLog(gpuInfo.deviceId, pmLog))
        {
            if (auto clk = Adl::ReadSensor(pmLog, Adl::SENSOR_CLK_GFXCLK); clk && *clk > 0)
                out.coreClockMhz = static_cast<uint32_t>(*clk);

            if (auto mem = Adl::ReadSensor(pmLog, Adl::SENSOR_CLK_MEMCLK); mem && *mem > 0)
                out.memClockMhz = static_cast<uint32_t>(*mem);

            if (auto hs = Adl::ReadSensor(pmLog, Adl::SENSOR_TEMP_HOTSPOT); hs && *hs > 0)
                out.hotspotC = static_cast<double>(*hs);

            if (auto temp = Adl::ReadSensor(pmLog, Adl::SENSOR_TEMP_EDGE); temp && *temp > 0)
                out.tempC = static_cast<double>(*temp);
            else if (out.hotspotC.has_value())
                out.tempC = out.hotspotC;
            else if (auto soc = Adl::ReadSensor(pmLog, Adl::SENSOR_TEMP_SOC); soc && *soc > 0)
                out.tempC = static_cast<double>(*soc);

            if (out.tempC.has_value())
                LogSource("GPU temperature", "ADL");

            if (auto mv = Adl::ReadSensor(pmLog, Adl::SENSOR_GFX_VOLTAGE); mv && *mv > 0)
                out.voltageV = static_cast<double>(*mv) / 1000.0;
            else if (auto socMv = Adl::ReadSensor(pmLog, Adl::SENSOR_SOC_VOLTAGE); socMv && *socMv > 0)
                out.voltageV = static_cast<double>(*socMv) / 1000.0;

            // RDNA4 reports total board power in SENSOR_BOARD_POWER (73), RDNA2/3 in SENSOR_ASIC_POWER (23) / GFX (30)
            if (auto pwr = Adl::ReadSensor(pmLog, Adl::SENSOR_BOARD_POWER); pwr && *pwr > 0)
                out.powerW = static_cast<double>(*pwr);
            else if (auto asic = Adl::ReadSensor(pmLog, Adl::SENSOR_ASIC_POWER); asic && *asic > 0)
                out.powerW = static_cast<double>(*asic);
            else if (auto gfx = Adl::ReadSensor(pmLog, Adl::SENSOR_GFX_POWER); gfx && *gfx > 0)
                out.powerW = static_cast<double>(*gfx);
            else if (auto apu = Adl::ReadSensor(pmLog, Adl::SENSOR_SSPAIRED_ASICPOWER); apu && *apu > 0)
                out.powerW = static_cast<double>(*apu);
        }
    }
    else if (gpuInfo.vendorId == VendorId::Nvidia)
    {
        Nvidia::Query(gpuInfo, out);

        if (out.tempC.has_value())
            LogSource("GPU temperature", "NVML / NVAPI");
    }

    // Under Proton there's no ADL, the kernel has amdgpu's sensors
    if (State::Instance().isRunningOnLinux && gpuInfo.vendorId == VendorId::AMD)
        Hwmon::QueryGpu(gpuInfo, out);

    if (!out.tempC.has_value() || !out.hotspotC.has_value() || !out.powerW.has_value() || !out.voltageV.has_value() ||
        !out.coreClockMhz.has_value() || !out.memClockMhz.has_value())
    {
        SharedSensors::QueryHwInfo(&out, nullptr, gpuInfo.name);
        SharedSensors::QueryAfterburner(&out, nullptr);
    }

    out.usagePercent = PdhGpu::QueryUsagePercent(gpuInfo.luid);

    return out;
}

inline CpuMetrics PollCpu(const GpuInformation& gpuInfo)
{
    CpuMetrics out {};

    // 1. AMD Adrenalin CPU Metrics Server (provides Clock, Voltage, Temp, Package Power in user mode), under Proton
    // the kernel's sensors
    if (State::Instance().isRunningOnLinux)
        Hwmon::QueryCpu(out);
    else
        SharedSensors::QueryAmdCpuServer(out);

    // 2. AMD APU sensors via ADL2 if any field is still missing
    if (!out.tempC.has_value() || !out.powerW.has_value() || !out.clockMhz.has_value())
    {
        if (gpuInfo.vendorId == VendorId::AMD)
        {
            Adl::ADLPMLogDataOutput pmLog {};
            if (Adl::PollPmLog(gpuInfo.deviceId, pmLog))
            {
                if (!out.tempC.has_value())
                {
                    if (auto t = Adl::ReadSensor(pmLog, Adl::SENSOR_TEMP_CPU); t && *t > 0)
                        out.tempC = static_cast<double>(*t);
                }
                if (!out.powerW.has_value())
                {
                    if (auto p = Adl::ReadSensor(pmLog, Adl::SENSOR_CPU_POWER); p && *p > 0)
                        out.powerW = static_cast<double>(*p);
                }
                if (!out.clockMhz.has_value())
                {
                    if (auto c = Adl::ReadSensor(pmLog, Adl::SENSOR_CLK_CPUCLK); c && *c > 0)
                        out.clockMhz = static_cast<uint32_t>(*c);
                }
            }
        }
    }

    // 3. HWiNFO64 / MSI Afterburner / CoreTemp shared memory fallback (Intel & AMD)
    if (!out.tempC.has_value() || !out.hotspotC.has_value() || !out.powerW.has_value() || !out.voltageV.has_value() ||
        !out.clockMhz.has_value())
    {
        SharedSensors::QueryHwInfo(nullptr, &out);
        SharedSensors::QueryAfterburner(nullptr, &out);
        SharedSensors::QueryCoreTemp(out);
    }

    // Once, so a log of a missing temperature says why
    static bool missingLogged = false;
    if (!out.tempC.has_value() && !missingLogged)
    {
        missingLogged = true;
        if (State::Instance().isRunningOnLinux)
            LOG_INFO("FPS overlay: no CPU temperature, no k10temp, zenpower or coretemp sensor in /sys/class/hwmon");
        else
            LOG_INFO("FPS overlay: no CPU temperature, it needs HWiNFO with Shared Memory Support, MSI Afterburner "
                     "with the CPU temperature monitored, Core Temp or AMD Adrenalin running");
    }

    // 4. Windows PDH effective CPU clock fallback (works on all Intel & AMD CPUs)
    if (!out.clockMhz.has_value())
        out.clockMhz = PdhCpu::QueryClockMhz();

    out.usagePercent = PdhCpu::QueryUsagePercent();

    return out;
}

inline std::optional<RamMetrics> PollRam()
{
    MEMORYSTATUSEX mem {};
    mem.dwLength = sizeof(mem);
    if (!GlobalMemoryStatusEx(&mem) || mem.ullTotalPhys == 0)
        return std::nullopt;

    RamMetrics out {};
    out.totalBytes = mem.ullTotalPhys;
    out.usedBytes = mem.ullTotalPhys > mem.ullAvailPhys ? mem.ullTotalPhys - mem.ullAvailPhys : 0;
    out.commitTotalBytes = mem.ullTotalPageFile;
    out.commitUsedBytes = mem.ullTotalPageFile > mem.ullAvailPageFile ? mem.ullTotalPageFile - mem.ullAvailPageFile : 0;

    PROCESS_MEMORY_COUNTERS_EX pmc {};
    pmc.cb = sizeof(pmc);
    if (K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
        out.gameBytes = pmc.WorkingSetSize;

    return out;
}

// Background worker so driver/PDH/shared-memory queries never run on the game's render/present thread
namespace AsyncWorker
{
constexpr uint64_t kIdleMs = 2000;
constexpr uint64_t kPollIntervalMs = 500;

struct Context
{
    std::mutex mutex;
    std::condition_variable cv;
    bool running = false;
    bool wakeNow = false;

    GpuInformation gpuInfo {};
    std::atomic<uint64_t> vramReqTick { 0 };
    std::atomic<uint64_t> gpuReqTick { 0 };
    std::atomic<uint64_t> cpuReqTick { 0 };
    std::atomic<uint64_t> ramReqTick { 0 };

    std::optional<std::pair<uint64_t, uint64_t>> vram;
    GpuMetrics gpu {};
    CpuMetrics cpu {};
    std::optional<RamMetrics> ram;
};

inline Context& Get()
{
    static Context ctx;
    return ctx;
}

inline void ThreadMain()
{
    auto& ctx = Get();
    std::unique_lock lock(ctx.mutex);

    while (true)
    {
        const uint64_t nowTick = GetTickCount64();
        const bool wantVram = ctx.vramReqTick.load() != 0 && nowTick - ctx.vramReqTick.load() <= kIdleMs;
        const bool wantGpu = ctx.gpuReqTick.load() != 0 && nowTick - ctx.gpuReqTick.load() <= kIdleMs;
        const bool wantCpu = ctx.cpuReqTick.load() != 0 && nowTick - ctx.cpuReqTick.load() <= kIdleMs;
        const bool wantRam = ctx.ramReqTick.load() != 0 && nowTick - ctx.ramReqTick.load() <= kIdleMs;

        if (!wantVram && !wantGpu && !wantCpu && !wantRam)
            break;

        const GpuInformation gpuInfo = ctx.gpuInfo;
        ctx.wakeNow = false;
        lock.unlock();

        std::optional<std::pair<uint64_t, uint64_t>> vramVal;
        GpuMetrics gpuVal {};
        CpuMetrics cpuVal {};
        std::optional<RamMetrics> ramVal;

        if (wantVram)
            vramVal = PollVram(gpuInfo);
        if (wantGpu)
            gpuVal = PollGpu(gpuInfo);
        if (wantCpu)
            cpuVal = PollCpu(gpuInfo);
        if (wantRam)
            ramVal = PollRam();

        lock.lock();
        if (wantVram)
            ctx.vram = vramVal;
        if (wantGpu)
            ctx.gpu = gpuVal;
        if (wantCpu)
            ctx.cpu = cpuVal;
        if (wantRam)
            ctx.ram = ramVal;

        ctx.cv.wait_for(lock, std::chrono::milliseconds(kPollIntervalMs), [&] { return ctx.wakeNow; });
    }

    ctx.running = false;
}

inline void EnsureRunning(Context& ctx, bool firstRequest)
{
    if (!ctx.running)
    {
        ctx.running = true;
        ctx.wakeNow = true;
        std::thread(ThreadMain).detach();
    }
    else if (firstRequest)
    {
        ctx.wakeNow = true;
        ctx.cv.notify_one();
    }
}
} // namespace AsyncWorker

// The display's vertical blanks, waited on by a thread of their own: with VRR the display refreshes when a frame is
// ready, so their rate is the refresh rate the display runs at (doubled frames below the VRR range included)
namespace VBlank
{
using D3dkmtHandle = UINT;

struct OpenAdapterFromHdc
{
    HDC hDc;
    D3dkmtHandle hAdapter;
    LUID adapterLuid;
    UINT vidPnSourceId;
};

struct WaitForVerticalBlankEvent
{
    D3dkmtHandle hAdapter;
    D3dkmtHandle hDevice;
    UINT vidPnSourceId;
};

struct CloseAdapter
{
    D3dkmtHandle hAdapter;
};

using PFN_OpenAdapterFromHdc = LONG(APIENTRY*)(OpenAdapterFromHdc*);
using PFN_WaitForVerticalBlankEvent = LONG(APIENTRY*)(const WaitForVerticalBlankEvent*);
using PFN_CloseAdapter = LONG(APIENTRY*)(const CloseAdapter*);

constexpr size_t kHistory = 1000;

struct Context
{
    std::mutex mutex;
    bool running = false;
    std::atomic<uint64_t> reqTick { 0 };
    std::atomic<HWND> window { nullptr };

    // QPC time of the last vertical blanks
    std::array<int64_t, kHistory> times {};
    size_t head = 0;
    size_t count = 0;
};

inline Context& Get()
{
    static Context ctx;
    return ctx;
}

inline void ThreadMain()
{
    auto& ctx = Get();

    static auto gdi = GetModuleHandleW(L"gdi32.dll");
    static auto openAdapter = gdi ? (PFN_OpenAdapterFromHdc) GetProcAddress(gdi, "D3DKMTOpenAdapterFromHdc") : nullptr;
    static auto waitVBlank =
        gdi ? (PFN_WaitForVerticalBlankEvent) GetProcAddress(gdi, "D3DKMTWaitForVerticalBlankEvent") : nullptr;
    static auto closeAdapter = gdi ? (PFN_CloseAdapter) GetProcAddress(gdi, "D3DKMTCloseAdapter") : nullptr;

    HMONITOR monitor = nullptr;
    WaitForVerticalBlankEvent wait {};
    uint64_t lastMonitorCheck = 0;

    while (openAdapter != nullptr && waitVBlank != nullptr && closeAdapter != nullptr)
    {
        const uint64_t nowTick = GetTickCount64();
        if (nowTick - ctx.reqTick.load() > AsyncWorker::kIdleMs)
            break;

        // The display the game's window is on, it can move
        if (nowTick - lastMonitorCheck > 1000)
        {
            lastMonitorCheck = nowTick;
            HMONITOR current = MonitorFromWindow(ctx.window.load(), MONITOR_DEFAULTTOPRIMARY);

            if (current != monitor)
            {
                if (wait.hAdapter != 0)
                {
                    CloseAdapter close { wait.hAdapter };
                    closeAdapter(&close);
                    wait = {};
                }

                monitor = current;
                MONITORINFOEXW info {};
                info.cbSize = sizeof(info);

                if (GetMonitorInfoW(monitor, &info))
                {
                    if (HDC dc = CreateDCW(nullptr, info.szDevice, nullptr, nullptr); dc != nullptr)
                    {
                        OpenAdapterFromHdc open { dc };
                        if (openAdapter(&open) == 0)
                            wait = { open.hAdapter, 0, open.vidPnSourceId };

                        DeleteDC(dc);
                    }
                }

                std::lock_guard lock(ctx.mutex);
                ctx.count = 0;
            }
        }

        if (wait.hAdapter == 0 || waitVBlank(&wait) != 0)
        {
            Sleep(100);
            continue;
        }

        LARGE_INTEGER now {};
        QueryPerformanceCounter(&now);

        std::lock_guard lock(ctx.mutex);
        ctx.times[ctx.head] = now.QuadPart;
        ctx.head = (ctx.head + 1) % kHistory;
        ctx.count = std::min(ctx.count + 1, kHistory);
    }

    if (wait.hAdapter != 0 && closeAdapter != nullptr)
    {
        CloseAdapter close { wait.hAdapter };
        closeAdapter(&close);
    }

    std::lock_guard lock(ctx.mutex);
    ctx.count = 0;
    ctx.running = false;
}
} // namespace VBlank
} // namespace detail

struct RefreshRate
{
    double currentHz = 0.0; // Last tenth of a second
    double averageHz = 0.0; // Last 1000 refreshes
};

// The display's actual refresh rate, from its vertical blanks
inline std::optional<RefreshRate> QueryRefreshRate(HWND window)
{
    auto& ctx = detail::VBlank::Get();
    ctx.reqTick = GetTickCount64();
    ctx.window = window;

    std::lock_guard lock(ctx.mutex);

    if (!ctx.running)
    {
        ctx.running = true;
        std::thread(detail::VBlank::ThreadMain).detach();
        return std::nullopt;
    }

    if (ctx.count < 2)
        return std::nullopt;

    LARGE_INTEGER frequency {};
    QueryPerformanceFrequency(&frequency);

    const size_t newest = (ctx.head + detail::VBlank::kHistory - 1) % detail::VBlank::kHistory;
    const size_t oldest = (ctx.head + detail::VBlank::kHistory - ctx.count) % detail::VBlank::kHistory;
    const int64_t newestTime = ctx.times[newest];

    RefreshRate out {};

    if (newestTime > ctx.times[oldest])
        out.averageHz = (ctx.count - 1) * static_cast<double>(frequency.QuadPart) / (newestTime - ctx.times[oldest]);

    // The refreshes of the last tenth of a second, long enough to even out the waits' wake up times
    size_t recent = 1;
    int64_t recentStart = newestTime;

    for (size_t i = 1; i < ctx.count; i++)
    {
        const int64_t time = ctx.times[(newest + detail::VBlank::kHistory - i) % detail::VBlank::kHistory];
        if (newestTime - time > frequency.QuadPart / 10)
            break;

        recent++;
        recentStart = time;
    }

    if (recent > 1 && newestTime > recentStart)
        out.currentHz = (recent - 1) * static_cast<double>(frequency.QuadPart) / (newestTime - recentStart);
    else
        out.currentHz = out.averageHz;

    return out;
}

inline std::optional<std::pair<uint64_t, uint64_t>> QueryVram(State& state)
{
    auto& ctx = detail::AsyncWorker::Get();
    const uint64_t nowTick = GetTickCount64();
    const uint64_t prevTick = ctx.vramReqTick.exchange(nowTick);
    const auto gpuInfo = detail::ActiveGpuInfo(state);

    std::lock_guard lock(ctx.mutex);
    ctx.gpuInfo = gpuInfo;
    detail::AsyncWorker::EnsureRunning(ctx, prevTick == 0 || nowTick - prevTick > detail::AsyncWorker::kIdleMs);
    return ctx.vram;
}

inline GpuMetrics QueryGpu(State& state)
{
    auto& ctx = detail::AsyncWorker::Get();
    const uint64_t nowTick = GetTickCount64();
    const uint64_t prevTick = ctx.gpuReqTick.exchange(nowTick);
    const auto gpuInfo = detail::ActiveGpuInfo(state);

    std::lock_guard lock(ctx.mutex);
    ctx.gpuInfo = gpuInfo;
    detail::AsyncWorker::EnsureRunning(ctx, prevTick == 0 || nowTick - prevTick > detail::AsyncWorker::kIdleMs);
    return ctx.gpu;
}

inline CpuMetrics QueryCpu(State& state)
{
    auto& ctx = detail::AsyncWorker::Get();
    const uint64_t nowTick = GetTickCount64();
    const uint64_t prevTick = ctx.cpuReqTick.exchange(nowTick);
    const auto gpuInfo = detail::ActiveGpuInfo(state);

    std::lock_guard lock(ctx.mutex);
    ctx.gpuInfo = gpuInfo;
    detail::AsyncWorker::EnsureRunning(ctx, prevTick == 0 || nowTick - prevTick > detail::AsyncWorker::kIdleMs);
    return ctx.cpu;
}

inline std::optional<RamMetrics> QueryRam()
{
    auto& ctx = detail::AsyncWorker::Get();
    const uint64_t nowTick = GetTickCount64();
    const uint64_t prevTick = ctx.ramReqTick.exchange(nowTick);

    std::lock_guard lock(ctx.mutex);
    detail::AsyncWorker::EnsureRunning(ctx, prevTick == 0 || nowTick - prevTick > detail::AsyncWorker::kIdleMs);
    return ctx.ram;
}
} // namespace OverlayTelemetry
