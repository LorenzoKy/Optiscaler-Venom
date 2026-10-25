<div align="center">
  <img src=".github/assets/changelog.svg" alt="OptiScaler Venom changelog" width="100%" />
</div>

<br />

What OptiScaler Venom changes compared to upstream [OptiScaler](https://github.com/optiscaler/OptiScaler), release by release. Upstream's own changes are in [Changelog.md](Changelog.md) and the [OptiScaler releases](https://github.com/optiscaler/OptiScaler/releases). Dates are UTC.

| Release | Date | Built on upstream `master` | |
| --- | --- | --- | --- |
| [v0.2 beta 3](#v02-beta-3) | 2026-10-25 | [`3c6263ec`](https://github.com/optiscaler/OptiScaler/commit/3c6263ec815fd50c09821cbfcf1411d77cb99396) (2026-10-25) | Latest |
| [v0.2 beta 2](#v02-beta-2) | 2026-10-25 | [`08e13adf`](https://github.com/optiscaler/OptiScaler/commit/08e13adf55b3fd71a67071eac2e95973ac598df3) (2026-10-24) | |
| [v0.2 beta 1](#v02-beta-1) | 2026-10-24 | [`539c9a1e`](https://github.com/optiscaler/OptiScaler/commit/539c9a1ecd2c37bee1980d0f0645919cab837855) (2026-10-24) | Pre-release |
| [v0.1](#v01) | 2026-10-24 | [`539c9a1e`](https://github.com/optiscaler/OptiScaler/commit/539c9a1ecd2c37bee1980d0f0645919cab837855) (2026-10-24) | |

Daily automatic builds, not tested, are listed under [Nightly builds](#nightly-builds).

---

## v0.2 beta 3

> **Built on OptiScaler `10.0.0.1`** (development build, after stable `v0.9.4`), upstream `master` at [`3c6263ec`](https://github.com/optiscaler/OptiScaler/commit/3c6263ec815fd50c09821cbfcf1411d77cb99396), 2026-10-25: everything in the official nightly [nightly-20261025](https://github.com/optiscaler/OptiScaler-nightly/releases/tag/nightly-20261025) plus the 4 upstream commits after it.

### New
- **Upscaler Name** option in **Row Options** (`FpsOverlayShowUpscalerName`, on by default): turn it off to hide the `DLSS -> DLSS 310.x` label on the FPS row ([#2](https://github.com/LorenzoKy/Optiscaler-Venom/issues/2)).
- From upstream: XeFG Dynamic MFG skips isolated hitches and keeps its count while the game is in the background; XeFG leaves the swapchain to the game's own FSR Frame Generation; AntiLag 2 frame types for XeFG presents; fix for a crash in Path of Exile 2 when switching V-Sync on Vulkan; the DLSSG input's hooks resolve `slSetFeatureLoaded`.

### Changed
- From upstream: overlay defaults a bit lighter (labels `0xEAEAEA`, background alpha 0.45, shadow 0.4, corners 2), the Low Latency Timings row with the overlay's shadow or outline.

### Fixed
- **GPU usage** missing on Linux (Proton/Wine) ([#2](https://github.com/LorenzoKy/Optiscaler-Venom/issues/2)): Wine has no GPU Engine counters, so the usage now comes from NVML or NVAPI (also through DXVK-NVAPI) on NVIDIA and from `amdgpu` on AMD. Windows still uses Task Manager's counters first.
- **Low Latency Timings** and **Frame Generation Time** rows no longer stay empty: they show `N/A` and why (no timings with the Reflex output, no markers, frame generation off or not timed).

### Known limits
- The Low Latency Timings row has no data with the Reflex output (upstream limit).
- Frame Generation Time only measures OptiScaler's D3D12 frame generation.
- On a single-CCD Ryzen (e.g. 7600) CPU Hotspot is the same as the CPU temperature: Tctl and the only CCD read the same.

## v0.2 beta 2

> **Built on OptiScaler `10.0.0.1`** (development build, after stable `v0.9.4`), upstream `master` at [`08e13adf`](https://github.com/optiscaler/OptiScaler/commit/08e13adf55b3fd71a67071eac2e95973ac598df3), 2026-10-24. Upstream now has the overlay customization that started in Venom ([optiscaler/OptiScaler#1238](https://github.com/optiscaler/OptiScaler/pull/1238)), so this release moves Venom onto upstream's version of it.

### New
- **GPU Hotspot** and **CPU Hotspot** options for the GPU and CPU rows (off by default, `FpsOverlayShowGpuHotspot`, `FpsOverlayShowCpuHotspot`). GPU: AMD junction temperature, NVIDIA hot spot (RTX 50 GPUs don't report it), or HWiNFO's / MSI Afterburner's. CPU: the hottest of AMD's Tctl and CCDs or of Intel's cores.
- **Temperatures on Linux** (Proton/Wine): CPU from the kernel's `k10temp`, `zenpower` or `coretemp` sensors, AMD GPU from `amdgpu` (temperature, junction, power, voltage, clocks).
- **Custom overlay type** from upstream, with every Venom row in it: choose the rows and their order in **Rows**. New **Frame Generation Time Graph** row.
- The log says where each temperature comes from, or what's missing when the CPU temperature can't be read.

### Changed
- The overlay types, presets and defaults are upstream's: **Full + Graph** by default, grey labels with a shadow, rounded corners. The Venom rows (latency, FG time, bottleneck, refresh rate, VRAM, GPU, CPU, RAM, date and time) show in the presets from Simple on when their option is on, under **Rows > More Rows in Presets**.
- Presets: upstream's *Default*, *Classic*, *Afterburner-Like* and *Glass Strip*. Venom's *Opti-Like* and *Latency Lab* are gone.
- **Metric Options** is now **Row Options**. **1% Low** only adds the 1% low, the average is part of the FPS row.
- Settings codes use upstream's format. Codes copied from v0.2 beta 1 can still be pasted.

### Fixed
- CPU temperature missing with an NVIDIA or Intel GPU ([#2](https://github.com/LorenzoKy/Optiscaler-Venom/issues/2)): MSI Afterburner's data was never read, and its GPU power, voltage and clocks were taken from the wrong readings.
- HWiNFO: the GPU readings come from the GPU the game runs on, not from an integrated GPU.
- The frame generation time row stayed empty with the Custom overlay type.

### Upgrading
- `FpsOverlayRowOrder`, `FpsOverlayShowFrameTime` and `FpsOverlayShowUpscalerTime` are gone: set `FpsOverlayType=7` (Custom) and list the rows in `FpsOverlayRows`, for example `fps,latency,frametime,frametimegraph,gpu,cpu`.
- With `auto` values the overlay now looks like upstream's new default (Full + Graph). Set `FpsOverlayType=0` for just the FPS as before.

### Known limits
- On Windows the CPU temperature needs one of these running: HWiNFO with **Shared Memory Support** on (the free version turns it off after 12 hours), MSI Afterburner with the CPU temperature in its monitoring list, Core Temp, or AMD Adrenalin (AMD GPUs).
- No NVIDIA hot spot on RTX 50 GPUs and on Linux: the driver doesn't report it there.

## v0.2 beta 1

> **Built on OptiScaler `10.0.0.1`** (development build, after stable `v0.9.4`), upstream `master` at [`539c9a1e`](https://github.com/optiscaler/OptiScaler/commit/539c9a1ecd2c37bee1980d0f0645919cab837855), 2026-10-24, same as v0.1. Pre-release.

### New
- **Frame Generation Time** overlay row (D3D12, off by default, `FpsOverlayShowFGTime`): GPU time of FSR FG, XeFG or DLSSG per real frame, average, time per generated frame and, with FPS Cost, the FPS it takes. Measured live with GPU timestamps around what the frame generation's libraries submit, on any direct or compute queue.
- **Extra dlls folder** (`OptiScaler\Extra dlls`, subfolders allowed): more DX12 FFX upscaler dlls (`amd_fidelityfx_upscaler_dx12.dll`, `amd_fidelityfx_dx12.dll`, `amdxcffx64.dll`). The versions they add are listed in the menu's FSR version list, named after their subfolder; versions already listed stay with the default dlls.

### Changed
- **Session timer** moved to the FPS row (same `FpsOverlayShowSessionTimer` key). The Date & Time row shows only the date and time.

### Fixed
- The overlay kept resizing and re-centering when a value changed length ([#1](https://github.com/LorenzoKy/Optiscaler-Venom/issues/1)): every row keeps its widest size until it has been narrower for 5 seconds.

### Known limits
- Only tested on Marvel's Spider-Man 2 with XeFG and FSR 4. FSR FG and DLSSG timing untested; Vulkan frame generation isn't timed.
- Frame generation work that runs next to the game's on another queue counts in full in the FG time.

## v0.1

> **Built on OptiScaler `10.0.0.1`** (development build, after stable `v0.9.4`), upstream `master` at [`539c9a1e`](https://github.com/optiscaler/OptiScaler/commit/539c9a1ecd2c37bee1980d0f0645919cab837855) "Make FFX Vk hooking side match the Dx12 side", 2026-10-24. It includes everything in the official nightly of that day.

First release: upstream OptiScaler plus a reworked FPS overlay. With default settings the overlay looks and behaves like upstream's.

### New: overlay metrics
- **Average, 1% low and 0.1% low FPS** on the FPS row (last 1000 and 5000 frames).
- **PC latency** (experimental), estimated like FrameView and PresentMon: from the input to the end of every shown frame, generated frames included, timed with GPU timestamps on D3D12. Uses the game's Reflex, XeLL or AntiLag 2 markers and PC Latency pings.
- **Render latency**: from simulation start to the end of the frame's GPU work. Show PC latency, render latency or both.
- **Bottleneck**: GPU Bound, CPU Bound, FPS Limit or Refresh Limit (V-Sync). Decided from the GPU's usage and how long the CPU waits for the GPU; a game's own FPS limit through Reflex, XeLL or AntiLag 2 shows as FPS Limit.
- **Real refresh rate** and its average (experimental), from the display's vertical blanks (follows VRR).
- **VRAM**: the game's use, the budget Windows gives it and the memory clock.
- **GPU** and **CPU**: temperature, power, voltage, clock, optionally with the model name and the **usage %** (as Task Manager shows it, any GPU vendor).
- **RAM**: system use, the game's working set, commit charge.
- **Date & Time** row in your Windows format, with an optional **session timer** (time since OptiScaler started).
- **Upscale ratio row** also shows the FSR 4 model and preset, or the overridden DLSS preset.
- **Upscaler FPS cost**: the FPS the upscaler's GPU time takes away.
- Frame time and upscaler time rows can be shown or hidden on any overlay type.

### New: customization
- Position: the 4 corners, top/bottom center, or any custom X/Y. Scale up to 3.0, independent from the menu scale.
- Font (any installed font or file), font size, letter spacing, label and value size.
- Label, value, graph and background colors; shadow or outline with strength and color; corner radius; graph height; row spacing.
- Row order, compact labels (UPS, PCL, FT...), update interval to make fast-changing values readable.
- **Presets**: Opti-Like, Afterburner-Like, Latency Lab, each with a readable font size.
- **Share Settings**: copy and paste the whole overlay setup as a code.
- Menu corner radius (`Rounding`).
- All of it is in the menu and documented in `OptiScaler.ini` (`FpsOverlay*` keys).

### Known limits
- PC latency and the refresh rate row are experimental. PC latency doesn't see the display's flip: expect a few ms of difference from FrameView or PresentMon. Without latency markers it falls back to an estimate.
- Hardware telemetry depends on what the GPU driver and the system expose: some values may be missing on some setups. CPU temperature and power may need HWiNFO, Afterburner or Core Temp running.
- An in-game FPS cap that doesn't go through Reflex, XeLL or AntiLag 2 shows as CPU Bound.

---

## Nightly builds

Automatic and **not tested**: the official OptiScaler nightly of the day plus the Venom changes (see the [README](README.md#-nightly-builds-always-up-to-date-not-tested)). Newest first; each line is added by the build itself.

Each build's page lists the OptiScaler changes since the previous nightly and the Venom changes in it. Only the last 14 nightlies stay downloadable.

| Date | Build | OptiScaler | Venom | Changes |
| --- | --- | --- | --- | --- |
<!-- nightly-rows -->
| 2026-10-24 | [**nightly 20261024**](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-nightly-20261024) | [`fd3a45a6`](https://github.com/optiscaler/OptiScaler/commit/fd3a45a6cac6d610a35c570479f80dae95ca4eac) · [nightly-20261024](https://github.com/optiscaler/OptiScaler-nightly/releases/tag/nightly-20261024) | [`8037f10a`](https://github.com/LorenzoKy/OptiScaler/commit/8037f10a7a5ce960493e6174902dfb85b5b2ea16) | 5 Venom |
