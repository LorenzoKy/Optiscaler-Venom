<img src="https://raw.githubusercontent.com/LorenzoKy/Optiscaler-Venom/opti-venom-v0.2/.github/assets/banner.svg" alt="OptiScaler Venom v0.2" width="100%" />

> **Built on OptiScaler `10.0.0.1` (development build, after stable `v0.9.4`)**: the latest official nightly [nightly-20261025](https://github.com/optiscaler/OptiScaler-nightly/releases/tag/nightly-20261025), upstream `master` at [`08e13adf`](https://github.com/optiscaler/OptiScaler/commit/08e13adf55b3fd71a67071eac2e95973ac598df3). Releases are now built on the official nightly, not on newer upstream commits.

The first stable v0.2: everything from beta 1 to beta 4 (frame generation time, Extra dlls, hotspots, Linux telemetry, XeFG in Vulkan games), plus:

### ✨ What's new
- **OSD Options** (was Row Options), now in three columns: FPS & Upscale, Hardware, Graphs. Much shorter.
- **A switch for every graph** in OSD Options: each graph shows under its row while that row is on, no more separate graph rows. Frame time and upscaler time are on by default like OptiScaler; the new **PC latency graph** (last 18 seconds) is off.
- **Upscale Inputs**: `DLSS -> FSR 4.0.3` moves to the Upscale Ratio row when that row is on, otherwise stays on the FPS row as in regular OptiScaler. Turn it off to hide it.

### ⚠️ Changed
- The upstream commits after nightly-20261025 that beta 3 and 4 had (XeFG Dynamic MFG and FSR FG fixes, AntiLag 2 frame types, Path of Exile 2 V-Sync crash fix, lighter OSD defaults) are not in v0.2: they come back with the next official nightly. The Venom nightlies have them.

### 🗑️ Removed
- **Refresh Rate / VRR** row: it wasn't accurate. Old ini files and settings codes that have it still work, it's just ignored.

### 📚 New guide
- The [README](https://github.com/LorenzoKy/Optiscaler-Venom#-how-to-use-the-venom-features) now explains every Venom feature step by step, **Extra dlls** included.
- 💬 Share your OSD codes and talk about Venom in the [**Opti Venom thread on Discord**](https://discord.com/channels/995299945492008990/1563930670018076843).

### 🛟 Who to ask

Open an [issue here](https://github.com/LorenzoKy/Optiscaler-Venom/issues) for any problem with this build. I only handle what is mine (**Venom**): upstream and third-party code is supported by its authors. More in the [README](https://github.com/LorenzoKy/Optiscaler-Venom/blob/main/README.md#-who-to-ask).

| Change | From | Git tag / ref | Ask |
| --- | --- | --- | --- |
| OSD Options, graph switches, PC latency graph, Upscale Inputs, refresh row removed | **Venom** | [`opti-venom-v0.2`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| XeFG in Vulkan games (since beta 4) | **dashdogy** | [optiscaler/OptiScaler#1240](https://github.com/optiscaler/OptiScaler/pull/1240), commits with `Venom-Part: dashdogy` | [dashdogy](https://github.com/dashdogy) on the PR |
| Everything else | **Upstream** | [nightly-20261025](https://github.com/optiscaler/OptiScaler-nightly/releases/tag/nightly-20261025), `master` [`08e13adf`](https://github.com/optiscaler/OptiScaler/commit/08e13adf55b3fd71a67071eac2e95973ac598df3) | [OptiScaler team](https://github.com/optiscaler/OptiScaler/issues) |

Full list in [CHANGELOG-Opti-Venom.md](https://github.com/LorenzoKy/Optiscaler-Venom/blob/opti-venom-v0.2/CHANGELOG-Opti-Venom.md).

### 📦 Install
Extract `OptiScaler-Venom_v0.2.7z` next to the game's executable and rename `OptiScaler.dll` as usual (e.g. `dxgi.dll`). Updating? Replace the DLL and keep your `OptiScaler.ini`, or take the new one for the new keys' notes. Guide: [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).
