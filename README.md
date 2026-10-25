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
  <a href="CHANGELOG-Opti-Venom.md"><img src="https://img.shields.io/badge/Changelog-v0.2%20beta%204-a78bfa?style=for-the-badge&logo=readthedocs&logoColor=white" alt="Changelog"></a>
  <a href="https://github.com/optiscaler/OptiScaler"><img src="https://img.shields.io/badge/Based%20on-OptiScaler-64748b?style=for-the-badge&logo=github" alt="Based on OptiScaler"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0-64748b?style=for-the-badge" alt="GPL-3.0"></a>

</div>

<br />

**OptiScaler Venom** (by LorenzoKy) is a community build of [OptiScaler](https://github.com/optiscaler/OptiScaler) that follows the latest upstream development and adds various features and additions that are unlikely to be seen in the regular Optiscaler.

> [!NOTE]
> This is an unofficial build, and I only look after the parts that are mine. **Open an issue [here](https://github.com/LorenzoKy/Optiscaler-Venom/issues)** for any problem with Venom, saying the release or nightly, the game, and attaching `OptiScaler.log`. **You can contact me ([@LorenzoKy](https://github.com/LorenzoKy)) only about code that comes from me**, the rows marked **Venom** below. Code that comes from upstream OptiScaler or from someone else (like dashdogy) is its authors' to support: [who to ask](#-who-to-ask). Please don't report problems of this build to the OptiScaler team unless they also happen with the official [nightly](https://github.com/optiscaler/OptiScaler/releases/tag/nightly).

## ✨ What's different, where it comes from and who to ask

Every feature, whose code it is, the git tag or commit to look at, and who to ask about problems with it. Anything not in this table is upstream OptiScaler, unchanged.

| Feature | Comes from | Git tag / ref | Ask |
| --- | --- | --- | --- |
| Upscalers, frame generation, spoofing | **Upstream** | [`v0.9.4`](https://github.com/optiscaler/OptiScaler/releases/tag/v0.9.4) and older, plus `master` at each release's commit | [OptiScaler team](https://github.com/optiscaler/OptiScaler/issues) |
| FPS overlay: FPS, frame time, upscaler time | **Upstream** | [`v0.9.4`](https://github.com/optiscaler/OptiScaler/releases/tag/v0.9.4) | [OptiScaler team](https://github.com/optiscaler/OptiScaler/issues) |
| Upscale ratio row | **Upstream** | `master` [`c4714282`](https://github.com/optiscaler/OptiScaler/commit/c4714282b222f5103b7d2e7ee3e4bd86dda05e96) (2026-10-20) | [OptiScaler team](https://github.com/optiscaler/OptiScaler/issues) |
| Fonts, colors, outline, rounding, any screen position, presets, copy/paste settings code | **Upstream** (started in Venom: [PR #1238](https://github.com/optiscaler/OptiScaler/pull/1238), merged) | `master` [`490dcb3b`](https://github.com/optiscaler/OptiScaler/commit/490dcb3b764ac19dfaa0bd66881f86fd397fdb1a) (2026-10-24). In Venom: [`opti-venom-v0.1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1), upstream's version since [`opti-venom-v0.2-beta2`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta2) | [OptiScaler team](https://github.com/optiscaler/OptiScaler/issues) |
| Custom overlay type, average FPS on the FPS row | **Upstream** | `master` [`9ec7ddb4`](https://github.com/optiscaler/OptiScaler/commit/9ec7ddb4f2cf6a20d51493ff4434c8fc29ec48a9) (2026-10-24), in Venom since [`opti-venom-v0.2-beta2`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta2) | [OptiScaler team](https://github.com/optiscaler/OptiScaler/issues) |
| XeFG in Vulkan games | **dashdogy** | [`opti-venom-v0.2-beta4`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta4), [optiscaler/OptiScaler#1240](https://github.com/optiscaler/OptiScaler/pull/1240) (not merged upstream), commits with `Venom-Part: dashdogy` | [dashdogy](https://github.com/dashdogy) on the PR |
| `[XeFG] Vulkan` switch, Vulkan presents that always return the image | **Venom** (around dashdogy's code) | [`opti-venom-v0.2-beta4`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta4) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| GPU and CPU hotspot, temperatures on Linux, GPU usage on Linux | **Venom** | [`opti-venom-v0.2-beta2`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta2), [`opti-venom-v0.2-beta3`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta3) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| Frame generation GPU time, Extra dlls folder | **Venom** | [`opti-venom-v0.2-beta1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.2-beta1) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| 1% and 0.1% low FPS | **Venom** | [`opti-venom-v0.1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| PC latency (PresentMon style, experimental) and render latency | **Venom** | [`opti-venom-v0.1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| Bottleneck from GPU usage and CPU wait: GPU / CPU / FPS limit / V-Sync | **Venom** | [`opti-venom-v0.1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| Real refresh rate, VRR aware (experimental) | **Venom** | [`opti-venom-v0.1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| VRAM, GPU, CPU and RAM telemetry, CPU & GPU usage % | **Venom** | [`opti-venom-v0.1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| Date, time and session timer | **Venom** | [`opti-venom-v0.1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |
| FSR 4 model / DLSS preset on the upscale ratio row | **Venom** | [`opti-venom-v0.1`](https://github.com/LorenzoKy/Optiscaler-Venom/releases/tag/opti-venom-v0.1) | [@LorenzoKy](https://github.com/LorenzoKy), [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues) |

With the default settings the overlay looks exactly like upstream's: nothing changes until you turn something on.

## 🛟 Who to ask

- **Venom**: code written by me. Its release sources are the `opti-venom-v*` tags of this repository. Open an [issue here](https://github.com/LorenzoKy/Optiscaler-Venom/issues) or write to [@LorenzoKy](https://github.com/LorenzoKy): this is the only code I support.
- **Upstream**: the code of the [OptiScaler team](https://github.com/optiscaler/OptiScaler) and its contributors, as it is on their `master`. It stays theirs even when it started in Venom and was merged into upstream afterwards. Check if the problem happens with the official [nightly](https://github.com/optiscaler/OptiScaler/releases/tag/nightly): if it does, report it in [their issues](https://github.com/optiscaler/OptiScaler/issues).
- **Third parties** (today only dashdogy): their code goes into the Venom patches as separate commits that keep their authorship and carry a `Venom-Part: <name>` line, in the [`opti-venom`](https://github.com/LorenzoKy/OptiScaler/tree/opti-venom) branch, so it can always be told apart and removed. Ask its author. I only answer for Venom's own switches and glue around it.

If you open an issue here about upstream or third-party code, it can be closed with a pointer to the right place.

Every release and nightly says in its notes which of its changes are Venom's, upstream's or someone else's.

## 📦 Installation

1. Download the latest `.7z` from [**Releases**](https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest).
2. Install it like regular OptiScaler: extract it next to the game's executable and rename `OptiScaler.dll` (for example to `dxgi.dll`). The full guide is in the [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).
3. Already using OptiScaler? Replace only `OptiScaler.dll` (renamed as before) and keep your `OptiScaler.ini`: the new overlay keys fall back to their defaults. To have them documented in your ini, take the one from the archive.

## 🌙 Nightly builds: always up to date, not tested

There are two kinds of downloads:

| | Stable releases | Nightly builds |
| --- | --- | --- |
| What | A version checked by me before publishing | The **official OptiScaler nightly of the day** + the Venom changes, built automatically |
| OptiScaler code | Upstream of the release date | The latest upstream changes, every day |
| Tested in game | ✅ | ❌ **Not tested** |
| Where | [**Latest release**](https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest) | [**Pre-releases**](https://github.com/LorenzoKy/Optiscaler-Venom/releases) named `OptiScaler Venom nightly YYYYMMDD` |

> [!WARNING]
> **Nightly builds are the way to always have OptiScaler's most recent changes, but nobody tests them.** They are built by a script every day from the [official nightly](https://github.com/optiscaler/OptiScaler-nightly/releases): if upstream breaks something, the nightly breaks too. Use them if you want the newest fixes and can live with bugs; otherwise stick to the stable releases. If a day's upstream changes conflict with the Venom ones, that day has no nightly. Every release has that day's official nightly in it, and every nightly has the latest release: on a day a release comes out there's no separate nightly.

Every nightly says exactly what it's made of: the upstream nightly and commit with their dates, and the Venom commit. The full list is in the [changelog](CHANGELOG-Opti-Venom.md#nightly-builds). Only the last 14 nightlies are kept.

## 🎛️ Using the overlay

Open the OptiScaler menu (`Insert` by default) and go to **FPS Overlay**:

- **Overlay Type**: the presets of rows from *Just FPS* to *Full + Graph*, or *Custom* to choose every row and its order in **Rows**. The Venom rows show in the presets when turned on in **Rows > More Rows in Presets**.
- **Row Options**: how the rows look (latency type, lows, CPU & GPU usage, GPU and CPU hotspot, session timer, hardware names).
- **Appearance > Presets**: *Default*, *Classic*, *Afterburner-Like* and *Glass Strip*, one click to apply, **Save Settings** to keep.
- **Appearance**: font, size, colors, shadow or outline, background, corner radius, position (corners, centers or custom X/Y).
- **Share Settings**: **Copy Code** puts your setup in the clipboard to share it, **Paste Code** applies someone else's.

Every option is also an `FpsOverlay*` key in `OptiScaler.ini`, each one documented there.

> [!TIP]
> **PC latency** needs a game with Reflex, XeLL or AntiLag 2. It's an estimate close to FrameView and PresentMon: expect a few milliseconds of difference, since the overlay can't see the display's flip.

## 📜 Changelog

See [**CHANGELOG-Opti-Venom.md**](CHANGELOG-Opti-Venom.md). Every stable release says which upstream OptiScaler version and commit it's built on, and every nightly build is listed there with its upstream nightly, commits and dates.

## 🙏 Credits

- **[OptiScaler](https://github.com/optiscaler/OptiScaler)** and its team and contributors (cdozdil, FakeMichau, TheRazerMD and everyone else): this build is entirely based on their work. If you like it, [support OptiScaler](https://github.com/sponsors/cdozdil?frequency=one-time).
- **[LorenzoKy](https://github.com/LorenzoKy)**: FPS overlay rework and the OptiScaler Venom builds.
- **[dashdogy](https://github.com/dashdogy)**: XeFG in Vulkan games ([PR #1240](https://github.com/optiscaler/OptiScaler/pull/1240)).
- The third-party libraries and SDKs OptiScaler uses, listed in the [original README](README-OptiScaler.md).

## ☕ Support

OptiScaler Venom is free and always will be. If it's useful to you and you'd like to say thanks, you can [buy me a coffee on Ko-fi](https://ko-fi.com/lorenzoky): completely optional, and appreciated. Supporting the [OptiScaler team](https://github.com/sponsors/cdozdil?frequency=one-time) helps the project this build is made of.

<a href="https://ko-fi.com/lorenzoky"><img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Support me on Ko-fi" /></a>

## ⚖️ License

GPL-3.0, like OptiScaler: see [LICENSE](LICENSE). The full source code of every release is in this repository, tagged with the release's version.
