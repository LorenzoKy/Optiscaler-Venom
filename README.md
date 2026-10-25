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

**OptiScaler Venom** (by LorenzoKy) is a community build of [OptiScaler](https://github.com/optiscaler/OptiScaler) that follows the latest upstream development and adds features that are unlikely to be seen in the regular OptiScaler: a much richer FPS overlay (OSD), frame generation timing, extra FSR dlls and more.

<div align="center">

**🧭 Jump to:** [🎬 Overview](#-overview) · [📦 Install](#-installation) · [🧪 How to use the Venom features](#-how-to-use-the-venom-features) · [📂 Extra dlls](#-extra-dlls-more-fsr-versions) · [📊 OSD](#-fps-overlay-osd) · [✨ What's different](#-whats-different) · [🛟 Who to ask](#-who-to-ask) · [🌙 Nightly](#-nightly-builds-always-up-to-date-not-tested) · [💬 Discord](#-discord) · [📜 Changelog](CHANGELOG-Opti-Venom.md)

</div>

> [!NOTE]
> Unofficial build: I only look after the parts that are mine. **Open an [issue here](https://github.com/LorenzoKy/Optiscaler-Venom/issues)** for any problem with Venom (release or nightly, game, and `OptiScaler.log` attached). Code from upstream OptiScaler or third parties is supported by its authors: see [who to ask](#-who-to-ask). Don't report Venom problems to the OptiScaler team unless they also happen with the official [nightly](https://github.com/optiscaler/OptiScaler/releases/tag/nightly).

## 🎬 Overview

What the Venom OSD looks like in game (Star Wars Outlaws):

https://github.com/user-attachments/assets/191a3007-0457-4b5d-a357-0ee303b0d583

## 📦 Installation

1. Download the latest `.7z` from [**Releases**](https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest).
2. Install it like regular OptiScaler: extract it next to the game's executable and rename `OptiScaler.dll` (for example to `dxgi.dll`). The full guide is in the [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).
3. Already using OptiScaler? Replace only `OptiScaler.dll` (renamed as before) and keep your `OptiScaler.ini`: the new keys fall back to their defaults. Take the archive's ini if you want them documented.

## 🧪 How to use the Venom features

Everything is in the OptiScaler menu (`Insert` by default). With default settings Venom looks and behaves like upstream OptiScaler: nothing changes until you turn it on.

### 📂 Extra dlls: more FSR versions

Use any FSR upscaler dll you want (for example FSR 4.0.3 next to the 4.1.x the game or the driver gives you), without replacing any file. DX12 only.

1. Open the game's folder, where `OptiScaler.dll` (renamed, e.g. `dxgi.dll`) is. Next to it there's the **`OptiScaler`** folder: create a folder called **`Extra dlls`** inside it.
   ```
   <game folder>\OptiScaler\Extra dlls\
   ```
2. Put the dlls in it, directly or in subfolders. A subfolder per version is the easiest way to keep several:
   ```
   <game folder>\OptiScaler\Extra dlls\4.0.3\amd_fidelityfx_upscaler_dx12.dll
   <game folder>\OptiScaler\Extra dlls\4.1.2\amd_fidelityfx_upscaler_dx12.dll
   ```
   Accepted files: `amd_fidelityfx_upscaler_dx12.dll` (FFX SDK upscaler), `amd_fidelityfx_dx12.dll` (FFX loader), `amdxcffx64.dll` (a copy of AMD's driver dll). Rename nothing.
3. Start the game, open the menu, choose an FSR upscaler (FSR 3.1 / 4) and pick the version from the **FFX Upscaler** list: the extra ones are at the end, named after their subfolder (for example `FSR 4.0.3 (4.0.3)`), or `(Extra)` when the dll is directly in `Extra dlls`.
4. Not in the list? A version the default dlls already have isn't listed twice, and `OptiScaler.log` says which dlls were loaded or skipped (search for `Extra`).

### 📊 FPS overlay (OSD)

Menu → **FPS Overlay**:

- **Overlay Type**: from *Just FPS* to *Full + Graph*, or *Custom* to choose every row and its order in **Rows**. Venom's rows (latency, FG time, bottleneck, VRAM, GPU, CPU, RAM, date and time) show in the presets when turned on in **Rows → More Rows in Presets**.
- **OSD Options**, in three columns:
  - **FPS & Upscale**: 1% and 0.1% lows, FPS cost of upscaler and frame generation, session timer, **Upscale Inputs** (`DLSS -> FSR 4.0.3`: on the Upscale Ratio row when that row is on, otherwise on the FPS row like upstream).
  - **Hardware**: model names, CPU & GPU usage, GPU and CPU hotspot, game RAM, commit charge.
  - **Graphs**: one switch per graph. Each graph shows under its row while that row is on: frame time, upscaler time (on by default, like OptiScaler), FG time and **PC latency** (off by default, the last 18 seconds). They are not rows of their own any more.
- **Latency Type**: PC latency (experimental, close to FrameView / PresentMon), render latency or both. Needs a game with Reflex, XeLL or AntiLag 2.
- **Appearance**: presets (*Default*, *Classic*, *Afterburner-Like*, *Glass Strip*), font, size, colors, shadow or outline, background, corners, position.
- **Share Settings**: **Copy Code** puts your whole OSD in the clipboard, **Paste Code** applies someone else's. Share yours in the [Discord thread](#-discord).

Every option is also an `FpsOverlay*` key in `OptiScaler.ini`, each one documented there.

### ⏱️ Frame generation time

Turn on **Frame Generation Time** (in **Rows → More Rows in Presets**, or in Custom's rows): the GPU time FSR FG, XeFG or DLSSG take per real frame and per generated frame, with **FPS Cost** the FPS they cost. D3D12 only.

### 🌋 XeFG in Vulkan games

On by default (by dashdogy): Vulkan games get XeFG, XeLL and Dynamic MFG like D3D12 games. If a Vulkan game breaks, set `[XeFG] Vulkan=false` in `OptiScaler.ini` and report it to dashdogy (see [who to ask](#-who-to-ask)).

## ✨ What's different

Anything not listed is upstream OptiScaler, unchanged. **Since** is the first Venom release with it, **From** says whose code it is and so [who to ask](#-who-to-ask).

| Feature | Since | From |
| --- | --- | --- |
| Lows (1% / 0.1%), PC and render latency, bottleneck, VRAM / GPU / CPU / RAM, usage %, date, time and session timer, FSR 4 model / DLSS preset, FPS cost | [v0.1][v0.1] | **Venom** |
| Frame generation GPU time, Extra dlls | [v0.2 beta 1][b1] | **Venom** |
| GPU / CPU hotspot, temperatures and GPU usage on Linux | [v0.2 beta 2][b2], [beta 3][b3] | **Venom** |
| `[XeFG] Vulkan` switch and safer Vulkan presents | [v0.2 beta 4][b4] | **Venom** |
| OSD Options columns, graph switches, PC latency graph, Upscale Inputs on the Upscale row | [v0.2][v0.2] | **Venom** |
| XeFG in Vulkan games ([PR #1240](https://github.com/optiscaler/OptiScaler/pull/1240), commits marked `Venom-Part: dashdogy`) | [v0.2 beta 4][b4] | **dashdogy** |
| OSD customization (fonts, colors, presets, settings codes), Custom overlay type, upscale ratio row | upstream `master`, started in Venom ([PR #1238](https://github.com/optiscaler/OptiScaler/pull/1238)) | **Upstream** |
| Upscalers, frame generation, spoofing, everything else | the official nightly each release is built on ([changelog](CHANGELOG-Opti-Venom.md)) | **Upstream** |

[v0.1]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1
[b1]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta1
[b2]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta2
[b3]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta3
[b4]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta4
[v0.2]: https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2

## 🛟 Who to ask

- **Venom**: my code. Open an [issue here](https://github.com/LorenzoKy/Optiscaler-Venom/issues) or write to [@LorenzoKy](https://github.com/LorenzoKy): the only code I support.
- **Upstream**: the [OptiScaler team](https://github.com/optiscaler/OptiScaler)'s code, also when it started in Venom. If it happens with the official [nightly](https://github.com/optiscaler/OptiScaler/releases/tag/nightly) too, report it in [their issues](https://github.com/optiscaler/OptiScaler/issues).
- **Third parties** (today dashdogy): separate commits that keep their authorship and carry a `Venom-Part: <name>` line in the [`opti-venom`](https://github.com/LorenzoKy/OptiScaler/tree/opti-venom) branch, so they can always be removed. Ask their author.

Issues about upstream or third-party code may be closed with a pointer to the right place. Every release and nightly says in its notes whose each change is.

## 🌙 Nightly builds: always up to date, not tested

| | Stable releases | Nightly builds |
| --- | --- | --- |
| What | Checked by me before publishing | The **official OptiScaler nightly of the day** + Venom, built automatically |
| Tested in game | ✅ | ❌ **Not tested** |
| Where | [**Latest release**](https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest) | [**Pre-releases**](https://github.com/LorenzoKy/Optiscaler-Venom/releases) named `OptiScaler Venom nightly YYYYMMDD` |

> [!WARNING]
> Releases are built on the latest **official OptiScaler nightly**, not on newer upstream commits.
>
> **Nobody tests the Venom nightlies.** If upstream breaks something that day, the nightly breaks too; if upstream's changes conflict with Venom's, there's no nightly that day. Every release has that day's official nightly in it and every nightly has the latest release. Only the last 14 are kept; the list is in the [changelog](CHANGELOG-Opti-Venom.md#nightly-builds).

## 💬 Discord

The **[Opti Venom thread](https://discord.com/channels/995299945492008990/1563930670018076843)** on the OptiScaler Discord server: share your OSD settings codes, show your setups and talk about Venom in general. Bugs still go to the [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues).

## 🙏 Credits

- **[OptiScaler](https://github.com/optiscaler/OptiScaler)** and its team and contributors (cdozdil, FakeMichau, TheRazerMD and everyone else): this build is entirely based on their work. If you like it, [support OptiScaler](https://github.com/sponsors/cdozdil?frequency=one-time).
- **[LorenzoKy](https://github.com/LorenzoKy)**: the Venom features and builds.
- **[dashdogy](https://github.com/dashdogy)**: XeFG in Vulkan games ([PR #1240](https://github.com/optiscaler/OptiScaler/pull/1240)).
- The third-party libraries and SDKs OptiScaler uses, listed in the [original README](README-OptiScaler.md).

## ☕ Support

OptiScaler Venom is free and always will be. If it's useful to you, you can [buy me a coffee on Ko-fi](https://ko-fi.com/lorenzoky): completely optional, and appreciated. Supporting the [OptiScaler team](https://github.com/sponsors/cdozdil?frequency=one-time) helps the project this build is made of.

## ⚖️ License

GPL-3.0, like OptiScaler: see [LICENSE](LICENSE). The full source code of every release is in this repository, tagged with the release's version.
