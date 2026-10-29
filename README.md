<div align="center">

  <img src=".github/assets/banner.svg" alt="OptiScaler Venom" width="100%" />

</div>
<hr />
<br />
<div align="center">
  <a href="https://ko-fi.com/lorenzoky"><img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Support me on Ko-fi" /></a>
</div>
<br />
<div align="center">

  <a href="https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest"><img src="https://img.shields.io/github/v/release/LorenzoKy/Optiscaler-Venom?style=for-the-badge&logo=github&label=Download&color=7c3aed" alt="Latest release"></a>
  <a href="CHANGELOG-Opti-Venom.md"><img src="https://img.shields.io/badge/Changelog-v0.2-a78bfa?style=for-the-badge&logo=readthedocs&logoColor=white" alt="Changelog"></a>
  <a href="https://discord.com/channels/995299945492008990/1563930670018076843"><img src="https://img.shields.io/badge/Discord-Opti%20Venom%20thread-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Opti Venom thread on Discord"></a>
  <a href="https://github.com/optiscaler/OptiScaler"><img src="https://img.shields.io/badge/Based%20on-OptiScaler-64748b?style=for-the-badge&logo=github" alt="Based on OptiScaler"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0-64748b?style=for-the-badge" alt="GPL-3.0"></a>

</div>

<br />

**OptiScaler Venom** (by LorenzoKy) is a community build of [OptiScaler](https://github.com/optiscaler/OptiScaler) that follows the latest upstream development and adds features that regular OptiScaler doesn't have (some of them may get there later, as the OSD rework did): more FPS overlay (OSD) rows and graphs, frame generation timing and more.
<div align="center">

**🧭 Jump to:** [🎬 Overview](#-overview) · [📦 Install](#-installation) · [🧪 How to use the Venom features](#-how-to-use-the-venom-features) · [📂 More FSR versions](#-more-fsr-versions-upstreams-custom-upscalers) · [📊 OSD](#-fps-overlay-osd) · [📏 Metrics explained](#-how-every-metric-works) · [✨ What's different](#-whats-different) · [🛟 Who to ask](#-who-to-ask) · [🌙 Nightly](#-nightly-builds-always-up-to-date-not-tested) · [💬 Discord](#-discord) · [📜 Changelog](CHANGELOG-Opti-Venom.md)

</div>

> [!NOTE]
> Unofficial build: I only look after the parts that are mine. **Open an [issue here](https://github.com/LorenzoKy/Optiscaler-Venom/issues)** for any problem with Venom (release or nightly, game, and `OptiScaler.log` attached). Code from upstream OptiScaler or third parties is supported by its authors: see [who to ask](#-who-to-ask). Don't report Venom problems to the OptiScaler team unless they also happen with the official [nightly](https://github.com/optiscaler/OptiScaler/releases/tag/nightly).

> [!IMPORTANT]
> **A passion project.** Venom is something I build in my free time because I enjoy it, and it's open to everyone. That also means **no guarantees**: fixes and support come when and if I can, and when and if I want to, not on request.
>
> **Want to help?** Ideas and contributions are welcome: open an issue or a pull request. I'll take in the ones I like and that fit the project. Indie devs, including those who work with AI, with good ideas can also reach me in the [Opti Venom thread](https://discord.com/channels/995299945492008990/1563930670018076843) on Discord.
>
> **Vibe coded.** Venom is a vibe coded project. And tokens are expensive: keep that in mind before asking for "just one more feature" 🙂

## 🎬 Overview

The FPS OSD rework of OptiScaler v10 started here and is created by me ([merge commit](https://github.com/optiscaler/OptiScaler/commit/08e13adf)).

Venom OSD Options:

https://github.com/user-attachments/assets/191a3007-0457-4b5d-a357-0ee303b0d583

## 📦 Installation

1. Download the latest `.7z` from [**Releases**](https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest).
2. Install it like regular OptiScaler: extract it next to the game's executable and rename `OptiScaler.dll` (for example to `dxgi.dll`). The full guide is in the [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).
3. Already using OptiScaler? Replace only `OptiScaler.dll` (renamed as before) and keep your `OptiScaler.ini`: the new keys fall back to their defaults. Take the archive's ini if you want them documented.

## 🧪 How to use the Venom features

Everything is in the OptiScaler menu (`Insert` by default). With default settings Venom looks and behaves like upstream OptiScaler: nothing changes until you turn it on.

### 📂 More FSR versions: upstream's custom upscalers

> [!NOTE]
> **Venom's Extra dlls are gone** since the nightly of 2026-10-29: upstream OptiScaler now loads more upscaler dlls by itself (custom upscalers), on DX12 and Vulkan, so Venom uses that. Releases up to v0.2 still have the `Extra dlls` folder. Copies of AMD's driver dll (`amdxcffx64.dll`) aren't loaded from a folder any more.

Use any FSR (or DLSS) upscaler dll you want, for example FSR 4.0.3 next to the 4.1.x the game or the driver gives you, without replacing any file:

1. In the game's folder, inside the **`OptiScaler`** folder next to `OptiScaler.dll` (renamed, e.g. `dxgi.dll`), create a **`custom`** folder with one folder per upscaler. Its name is the name you'll see in the menu.
2. Put the dll in the subfolder for its kind:
   ```
   <game folder>\OptiScaler\custom\FSR 4.0.3\ffx\amd_fidelityfx_upscaler_dx12.dll      (DX12 FFX)
   <game folder>\OptiScaler\custom\FSR 4.0.3\ffx_vk\amd_fidelityfx_upscaler_vk.dll    (Vulkan FFX)
   <game folder>\OptiScaler\custom\My DLSS\dlss\nvngx.dll                             (NGX runtime for DLSS)
   ```
   The dll can have any name; with more dlls in the folder the standard name is used.
3. Start the game, open the menu and pick it in the upscaler list: each folder is its own entry.

Moving from `Extra dlls`: `Extra dlls\4.0.3\amd_fidelityfx_upscaler_dx12.dll` becomes `custom\4.0.3\ffx\amd_fidelityfx_upscaler_dx12.dll`.

### 📊 FPS overlay (OSD)

Menu → **FPS Overlay**:

- **Overlay Type**: from *Just FPS* to *Full + Graph*, or *Custom* to choose every row and its order in **Rows**. Venom's rows (latency, FG time, bottleneck, VRAM, GPU, CPU, RAM, date and time) show in the presets when turned on in **Rows → More Rows in Presets**.
- **OSD Options**, in three columns:
  - **FPS & Upscale**: 1% and 0.1% lows, FPS cost of upscaler and frame generation, session timer, **Upscale Inputs** (`DLSS -> FSR 4.0.3`: on the Upscale Ratio row when that row is on, otherwise on the FPS row like upstream).
  - **Hardware**: model names, CPU & GPU usage, GPU and CPU hotspot, game RAM, commit charge.
  - **Graphs**: one switch per graph. Each graph shows under its row while that row is on: frame time, upscaler time (on by default, like OptiScaler), FG time and **PC latency** (off by default, the last 18 seconds). They are not rows of their own any more.
- **Latency Type**: PC latency (experimental, meant to match FrameView / PresentMon's, usually within a few ms), render latency or both. Needs a game with Reflex, XeLL or AntiLag 2.
- **Appearance**: presets (*Default*, *Classic*, *Afterburner-Like*, *Glass Strip*), font, size, colors, shadow or outline, background, corners, position.
- **Share Settings**: **Copy Code** puts your whole OSD in the clipboard, **Paste Code** applies someone else's. Share yours in the [Discord thread](#-discord).

Every option is also an `FpsOverlay*` key in `OptiScaler.ini`, each one documented there.

### ⏱️ Frame generation time

Turn on **Frame Generation Time** (in **Rows → More Rows in Presets**, or in Custom's rows): the GPU time FSR FG, XeFG or DLSSG take per real frame and per generated frame, with **FPS Cost** the FPS they cost. D3D12 only.

### 🌋 XeFG in Vulkan games

On by default (by dashdogy): Vulkan games get XeFG, XeLL and Dynamic MFG like D3D12 games. If a Vulkan game breaks, set `[XeFG] Vulkan=false` in `OptiScaler.ini` and report it to dashdogy (see [who to ask](#-who-to-ask)).

## 📏 How every metric works

What each OSD value measures and where it comes from, so you know what you're looking at and why it can differ from other tools.

### ⚡ PC latency and render latency

Both need a game with **Reflex, XeLL or AntiLag 2** (its markers tell OptiScaler when each frame's simulation starts). Choose what the row shows in **Latency Type**: PC latency, render latency or both.

- **Render latency**: from the start of the frame's simulation (the game reading input and updating the world) to the end of its GPU work, frame generation included. Venom takes the best source available:
  1. a GPU fence it places right after the game's present: the real moment the GPU finishes the frame;
  2. otherwise the Reflex driver's own reports (they stop at the game's GPU work, before frame generation);
  3. otherwise an **estimate** from the CPU-side markers only, up to the end of the present: shown as `(est.)`, it doesn't include the GPU time and reads lower than reality.
- **PC latency** (experimental): the number tools like **NVIDIA FrameView** and **PresentMon** call PC latency, the time from your input to the frame on its way to the screen. It's the sum of two parts:
  1. **input wait**: how long an input waits before the game's next simulation start picks it up. Venom measures it with pings like Reflex's PC Latency Stats; without pings it uses half a real frame, the average wait;
  2. **simulation to display**: from the simulation start to the end of the GPU work of **every frame shown**, generated frames included (with frame generation one real frame becomes several shown ones, each with its own latency).

  PC latency isn't shown when only the estimate is available, because it would be wrong.

**Why it doesn't match other tools exactly:** Venom stops at the end of the GPU work and doesn't see the display's flip (scan-out, the monitor's own processing), so expect a few ms of difference from FrameView or PresentMon. It also averages the last frames and the text updates every 250 ms; the **latency graph** takes a value every 50 ms and shows the last 18 seconds. Neither is end-to-end latency (mouse click to photons): that needs hardware like NVIDIA's LDAT or a high-speed camera.

The `| Reflex` / `| XeLL` / `| AntiLag 2` after the value is the low latency method in use.

### 🎞️ FPS, frame time and lows

- **FPS / frame time**: from the time between the game's presents. With frame generation it counts the frames shown, generated included. **Average FPS** averages them over time.
- **1% low**: the FPS of the 99th percentile frame time over the last 1000 frames, the way FrameView calculates it (not the average of the slowest 1%). **0.1% low**: the same at the 99.9th percentile over the last 5000 frames. Recalculated twice a second; they react to stutters, not to one isolated hitch.
- **Session timer**: how long OptiScaler has been running in the game.

### 🔍 Upscaler and frame generation

- **Upscale ratio**: render resolution → output resolution, updated in real time, so dynamic resolution shows.
- **Upscaler time**: the upscaler's GPU time, as OptiScaler measures it (upstream's row).
- **Frame generation time** (D3D12, FSR FG / XeFG / DLSSG): GPU timestamps around everything the frame generation library submits (interpolation, the swapchain's copies and composition), per real and per generated frame. Work running on another queue next to the game's counts in full, so it can be higher than what it really costs in FPS.
- **FPS cost**: how many output FPS you'd gain without the upscaler's or the frame generation's GPU time. It assumes the GPU is the limit: when you're CPU bound the real cost is lower.

### 🚦 Bottleneck

What limits the frame rate: **GPU Bound**, **CPU Bound**, **FPS Limit** or **Refresh Limit** (V-Sync). It's deduced from the GPU's usage and from how long the game waits for the GPU each frame, in the low latency sleep and in the present. An FPS limit set through Reflex, XeLL or AntiLag 2 shows as FPS Limit; other in-game limiters look CPU Bound, because from outside they look the same.

### 🖥️ Hardware

- **VRAM**: the game's video memory use, the budget Windows gives it and the memory clock, from DXGI / the driver.
- **GPU**: temperature, power, voltage and core clock from the GPU's driver; **hotspot** is the hottest sensor (AMD junction, NVIDIA hot spot; RTX 50 GPUs don't report it, then HWiNFO or Afterburner are used).
- **CPU**: temperature, power, voltage and clock. Windows doesn't give CPU temperatures to apps, so they come from **HWiNFO** (with Shared Memory Support on), **MSI Afterburner**, **Core Temp** or **AMD Adrenalin** running in the background; on Linux (Proton/Wine) from the kernel's hwmon sensors. Nothing running = no temperature (the clock still shows, from Windows' counters). Power and voltage come from the same apps. **Hotspot** is the hottest core or CCD.
- **Usage %**: GPU and CPU usage as Task Manager shows it.
- **RAM**: system RAM in use, plus the game's own working set and, if on, the commit charge (RAM + pagefile).
- **Date and time**: in your Windows format.

## ✨ What's different

Anything not listed is upstream OptiScaler, unchanged. **Since** is the first Venom release with it, **From** says whose code it is and so [who to ask](#-who-to-ask).

| Feature | Since | From |
| --- | --- | --- |
| Lows (1% / 0.1%), PC and render latency, bottleneck, VRAM / GPU / CPU / RAM, usage %, date, time and session timer, FSR 4 model / DLSS preset, FPS cost | [v0.1][v0.1] | **Venom** |
| Frame generation GPU time (Extra dlls until v0.2, replaced by upstream's custom upscalers) | [v0.2 beta 1][b1] | **Venom** |
| GPU / CPU hotspot, temperatures and GPU usage on Linux | [v0.2 beta 2][b2], [beta 3][b3] | **Venom** |
| `[XeFG] Vulkan` switch and safer Vulkan presents | [v0.2 beta 4][b4] | **Venom** |
| OSD Options columns, graph switches, PC latency graph, Upscale Inputs on the Upscale row | [v0.2][v0.2] | **Venom** |
| XeFG in Vulkan games ([PR #1240](https://github.com/optiscaler/OptiScaler/pull/1240), commits marked `Venom-Part: dashdogy`) | [v0.2 beta 4][b4] | **dashdogy** |
| OSD customization (fonts, colors, presets, settings codes), Custom overlay type, upscale ratio row | upstream `master`, started in Venom ([PR #1238](https://github.com/optiscaler/OptiScaler/pull/1238)) | **Upstream** |
| Custom upscalers (`OptiScaler\custom` folder), in place of Venom's Extra dlls | nightly 2026-10-29 | **Upstream** |
| Upscalers, frame generation, spoofing, everything else | the official nightly each release is built on ([changelog](CHANGELOG-Opti-Venom.md)) | **Upstream** |

[v0.1]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1
[b1]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta1
[b2]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta2
[b3]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta3
[b4]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta4
[v0.2]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2

## 🛟 Who to ask

- **Venom**: my code. Open an [issue here](https://github.com/LorenzoKy/Optiscaler-Venom/issues) or write to [@LorenzoKy](https://github.com/LorenzoKy): the only code I look after (with the limits of the passion project note at the top).
- **Upstream**: the [OptiScaler team](https://github.com/optiscaler/OptiScaler)'s code, also when it started in Venom. If it happens with the official [nightly](https://github.com/optiscaler/OptiScaler/releases/tag/nightly) too, report it in [their issues](https://github.com/optiscaler/OptiScaler/issues).
- **Third parties** (today dashdogy): separate commits that keep their authorship and carry a `Venom-Part: <name>` line in the [`opti-venom`](https://github.com/LorenzoKy/OptiScaler/tree/opti-venom) branch, so they can always be removed. Ask their author.

Issues about upstream or third-party code may be closed with a pointer to the right place. Every release and nightly says in its notes whose each change is.

## 🌙 Nightly builds: always up to date, not tested

| | Stable releases | Nightly builds |
| --- | --- | --- |
| What | Checked by me before publishing | The **official OptiScaler nightly of the day** + Venom, built automatically |
| Tested in game | ✅ (in the games I have, not every game) | ❌ **Not tested** |
| Where | [**Latest release**](https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest) | [**Pre-releases**](https://github.com/LorenzoKy/Optiscaler-Venom/releases) named `OptiScaler Venom nightly YYYYMMDD` |

> [!WARNING]
> Releases are built on the latest **official OptiScaler nightly**, not on newer upstream commits.
>
> **Nobody tests the Venom nightlies.** If upstream breaks something that day, the nightly breaks too; if upstream's changes conflict with Venom's, there's no nightly that day. Every release has that day's official nightly in it and every nightly has the latest release. Only the last 14 are kept; the list is in the [changelog](CHANGELOG-Opti-Venom.md#nightly-builds).

## 💬 Discord

The **[Opti Venom thread](https://discord.com/channels/995299945492008990/1563930670018076843)** on the OptiScaler Discord server: share your OSD settings codes, show your setups and talk about Venom in general. Bugs still go to the [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues).

## 🙏 Credits

- **[OptiScaler](https://github.com/optiscaler/OptiScaler)** and its team and contributors (cdozdil, FakeMichau, TheRazerMD and everyone else): this build is entirely based on their work. If you like it, [support OptiScaler](https://github.com/sponsors/cdozdil?frequency=one-time).
- **[LorenzoKy](https://github.com/LorenzoKy)**: the Venom features and builds, and the FPS OSD rework of OptiScaler v10 ([merge commit](https://github.com/optiscaler/OptiScaler/commit/08e13adf)).
- **[dashdogy](https://github.com/dashdogy)**: XeFG in Vulkan games ([PR #1240](https://github.com/optiscaler/OptiScaler/pull/1240)).
- The third-party libraries and SDKs OptiScaler uses, listed in the [original README](README-OptiScaler.md).

## ☕ Support

OptiScaler Venom is free and always will be. If it's useful to you, you can [buy me a coffee on Ko-fi](https://ko-fi.com/lorenzoky): completely optional, and appreciated. Supporting the [OptiScaler team](https://github.com/sponsors/cdozdil?frequency=one-time) helps the project this build is made of.

## ⚖️ License

GPL-3.0, like OptiScaler: see [LICENSE](LICENSE). The full source code of every release is in this repository, tagged with the release's version.
