<img src="https://raw.githubusercontent.com/LorenzoKy/Optiscaler-Venom/opti-venom-v0.2-beta3/.github/assets/banner.svg" alt="OptiScaler Venom v0.2 beta 3" width="100%" />

> **Built on OptiScaler `10.0.0.1` (development build, after stable `v0.9.4`)**: upstream `master` at [`3c6263ec`](https://github.com/optiscaler/OptiScaler/commit/3c6263ec815fd50c09821cbfcf1411d77cb99396), 2026-10-25. It has everything in today's official nightly [nightly-20261025](https://github.com/optiscaler/OptiScaler-nightly/releases/tag/nightly-20261025) plus the 4 upstream commits after it.

### ✨ What's new
- **Upscaler Name** in **Row Options** (`FpsOverlayShowUpscalerName`, on by default): turn it off to hide the `DLSS -> DLSS 310.x` label on the FPS row.
- From upstream: XeFG Dynamic MFG fixes (isolated hitches skipped, count kept in the background), the game's own FSR FG keeps its swapchain, AntiLag 2 frame types for XeFG, a V-Sync crash fix in Path of Exile 2 on Vulkan.

### 🐛 Fixes
- **GPU usage on Linux** ([#2](https://github.com/LorenzoKy/Optiscaler-Venom/issues/2)): Wine has no GPU Engine counters, so the usage now comes from NVML / NVAPI on NVIDIA (DXVK-NVAPI too) and from `amdgpu` on AMD.
- **Low Latency Timings** and **Frame Generation Time** rows show `N/A` and the reason instead of staying empty.

### 🔄 Changes
- Upstream's lighter overlay defaults: labels `0xEAEAEA`, background alpha 0.45, shadow 0.4, corner radius 2.

### ⚠️ Notes
- Low Latency Timings have no data with the Reflex output, Frame Generation Time only measures OptiScaler's D3D12 frame generation.
- On a single-CCD Ryzen CPU Hotspot equals the CPU temperature: Tctl and the only CCD read the same.

### 🛟 Who to ask

Open an [issue here](https://github.com/LorenzoKy/Optiscaler-Venom/issues) for any problem with this build. I only handle what is mine (**Venom**): upstream and third-party code is supported by its authors, so ask them as the table says. More in the [README](https://github.com/LorenzoKy/Optiscaler-Venom/blob/main/README.md#-who-to-ask).

| Change | From | Git tag / ref | Ask |
| --- | --- | --- | --- |
| Upscaler Name option, GPU usage on Linux, `N/A` rows for Low Latency Timings and Frame Generation Time | **Venom** | [`opti-venom-v0.2-beta3`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta3) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| XeFG Dynamic MFG fixes, FSR FG swapchain, AntiLag 2 frame types, Path of Exile 2 V-Sync crash fix, DLSSG hooks, lighter overlay defaults, and everything else | **Upstream** | `master` [`3c6263ec`](https://github.com/optiscaler/OptiScaler/commit/3c6263ec815fd50c09821cbfcf1411d77cb99396) | [OptiScaler team](https://github.com/optiscaler/OptiScaler/issues) |

Full list in [CHANGELOG-Opti-Venom.md](https://github.com/LorenzoKy/Optiscaler-Venom/blob/opti-venom-v0.2-beta3/CHANGELOG-Opti-Venom.md).

### 📦 Install
Extract `OptiScaler-Venom_v0.2-beta3.7z` next to the game's executable and rename `OptiScaler.dll` as usual (e.g. `dxgi.dll`). Updating? Replace the DLL and take the new `OptiScaler.ini` for the new keys' notes, or keep yours. Guide: [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).
