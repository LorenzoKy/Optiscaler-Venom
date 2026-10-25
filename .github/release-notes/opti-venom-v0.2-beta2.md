<img src="https://raw.githubusercontent.com/LorenzoKy/Optiscaler-Venom/opti-venom-v0.2-beta2/.github/assets/banner.svg" alt="OptiScaler Venom v0.2 beta 2" width="100%" />

> **Built on OptiScaler `10.0.0.1` (development build, after stable `v0.9.4`)**: upstream `master` at [`08e13adf`](https://github.com/optiscaler/OptiScaler/commit/08e13adf55b3fd71a67071eac2e95973ac598df3), 2026-10-24. Upstream now has the overlay customization that started in Venom, so Venom moves onto upstream's version of it.

### ✨ What's new
- **GPU Hotspot** and **CPU Hotspot** on the GPU and CPU rows, off by default (`FpsOverlayShowGpuHotspot`, `FpsOverlayShowCpuHotspot`, or **Row Options** in the menu). GPU: AMD junction temperature or NVIDIA hot spot (not on RTX 50). CPU: the hottest core or CCD.
- **Temperatures on Linux** (Proton/Wine): CPU from the kernel's sensors (`k10temp`, `zenpower`, `coretemp`), AMD GPU from `amdgpu`.
- **Custom overlay type**: pick any rows in any order, Venom's included, in **Rows**.

### 🐛 Fixes
- CPU temperature missing with NVIDIA and Intel GPUs ([#2](https://github.com/LorenzoKy/Optiscaler-Venom/issues/2)): MSI Afterburner's data was never read. Its GPU power, voltage and clocks were also wrong.
- HWiNFO's GPU readings no longer come from an integrated GPU.
- Frame generation time row empty with the Custom overlay type.

### 🔄 Changes
- Overlay types, presets and defaults are upstream's. With `auto` settings the overlay is now **Full + Graph** with grey labels and a shadow; `FpsOverlayType=0` gives just the FPS as before.
- `FpsOverlayRowOrder`, `FpsOverlayShowFrameTime` and `FpsOverlayShowUpscalerTime` are gone: use the Custom type (`FpsOverlayType=7`) with `FpsOverlayRows`.
- Venom's *Opti-Like* and *Latency Lab* presets are replaced by upstream's. Settings codes from v0.2 beta 1 still paste.

### ⚠️ Notes
- On Windows the CPU temperature needs HWiNFO (with **Shared Memory Support** on), MSI Afterburner (CPU temperature in its monitoring list), Core Temp or AMD Adrenalin running.

### 🛟 Who to ask

Open an [issue here](https://github.com/LorenzoKy/Optiscaler-Venom/issues) for any problem with this build. I only handle what is mine (**Venom**): upstream and third-party code is supported by its authors, so ask them as the table says. More in the [README](https://github.com/LorenzoKy/Optiscaler-Venom/blob/main/README.md#-who-to-ask).

| Change | From | Git tag / ref | Ask |
| --- | --- | --- | --- |
| GPU and CPU hotspot, temperatures on Linux, CPU temperature with NVIDIA / Intel GPUs, HWiNFO GPU pick, Frame Generation Time Graph row, import of beta 1 settings codes | **Venom** | [`opti-venom-v0.2-beta2`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta2) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| Custom overlay type, overlay types, presets and defaults, overlay customization (started in Venom: [PR #1238](https://github.com/optiscaler/OptiScaler/pull/1238), merged), and everything else | **Upstream** | `master` [`08e13adf`](https://github.com/optiscaler/OptiScaler/commit/08e13adf55b3fd71a67071eac2e95973ac598df3) | [OptiScaler team](https://github.com/optiscaler/OptiScaler/issues) |

Full list in [CHANGELOG-Opti-Venom.md](https://github.com/LorenzoKy/Optiscaler-Venom/blob/opti-venom-v0.2-beta2/CHANGELOG-Opti-Venom.md).

### 📦 Install
Extract `OptiScaler-Venom_v0.2-beta2.7z` next to the game's executable and rename `OptiScaler.dll` as usual (e.g. `dxgi.dll`). Updating? Replace the DLL and take the new `OptiScaler.ini` for the new keys' notes, or keep yours. Guide: [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).
