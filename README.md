<div align="center">

  <img src=".github/assets/banner.svg" alt="OptiScaler Venom" width="100%" />

</div>
<hr />
<br />
<div align="center">
  <a href="https://ko-fi.com/lorenzoky"><img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Support me on Ko-fi" /></a>
<div align="center">

  <a href="https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest"><img src="https://img.shields.io/github/v/release/LorenzoKy/Optiscaler-Venom?style=for-the-badge&logo=github&label=Download&color=7c3aed" alt="Latest release"></a>
  <a href="CHANGELOG-Opti-Venom.md"><img src="https://img.shields.io/badge/Changelog-v0.1-a78bfa?style=for-the-badge&logo=readthedocs&logoColor=white" alt="Changelog"></a>
  <a href="https://github.com/optiscaler/OptiScaler"><img src="https://img.shields.io/badge/Based%20on-OptiScaler-64748b?style=for-the-badge&logo=github" alt="Based on OptiScaler"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0-64748b?style=for-the-badge" alt="GPL-3.0"></a>

</div>

<br />

**OptiScaler Venom** (by LorenzoKy, LKY) is a community build of [OptiScaler](https://github.com/optiscaler/OptiScaler) that follows the latest upstream development and adds a reworked **FPS overlay**: every row, color, font and position can be changed, and it shows a lot more than FPS. Everything else (upscalers, frame generation, spoofing, the in-game menu) is upstream OptiScaler, unchanged.

> [!NOTE]
> This is an unofficial build. Please don't report issues with it to the OptiScaler team: open them [here](https://github.com/LorenzoKy/Optiscaler-Venom/issues) instead. If a problem also happens with the official [nightly](https://github.com/optiscaler/OptiScaler/releases/tag/nightly), it's an upstream one.

## ✨ What's different

| | Upstream OptiScaler | OptiScaler Venom |
| --- | :---: | :---: |
| Upscalers, frame generation, spoofing | ✅ | ✅ (same code) |
| FPS overlay: FPS, frame time, upscaler time | ✅ | ✅ |
| Average, 1% and 0.1% low FPS | | ✅ |
| PC latency (PresentMon style, experimental) and render latency | | ✅ |
| Bottleneck from GPU usage and CPU wait: GPU / CPU / FPS limit / V-Sync | | ✅ |
| Real refresh rate, VRR aware (experimental) | | ✅ |
| VRAM, GPU, CPU and RAM telemetry, CPU & GPU usage % | | ✅ |
| Date, time and session timer | | ✅ |
| Upscale ratio with FSR 4 model / DLSS preset | | ✅ |
| Fonts, colors, outline, rounding, row order, any screen position | | ✅ |
| Ready-made overlay presets, copy/paste settings code | | ✅ |

With the default settings the overlay looks exactly like upstream's: nothing changes until you turn something on.

## 📦 Installation

1. Download the latest `.7z` from [**Releases**](https://github.com/LorenzoKy/Optiscaler-Venom/releases/latest).
2. Install it like regular OptiScaler: extract it next to the game's executable and rename `OptiScaler.dll` (for example to `dxgi.dll`). The full guide is in the [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).
3. Already using OptiScaler? Replace only `OptiScaler.dll` (renamed as before) and keep your `OptiScaler.ini`: the new overlay keys fall back to their defaults. To have them documented in your ini, take the one from the archive.

## 🎛️ Using the overlay

Open the OptiScaler menu (`Insert` by default) and go to **FPS Overlay**:

- **Presets**: *Opti-Like*, *Afterburner-Like* and *Latency Lab*, one click to apply, **Save Settings** to keep.
- **Rows** and **Metric Options**: choose what to show, in which order, and how (latency type, lows, CPU & GPU usage, session timer, hardware names, compact labels).
- **Appearance**: font, size, colors, shadow or outline, background, corner radius, position (corners, centers or custom X/Y).
- **Share Settings**: **Copy Code** puts your setup in the clipboard to share it, **Paste Code** applies someone else's.

Every option is also an `FpsOverlay*` key in `OptiScaler.ini`, each one documented there.

> [!TIP]
> **PC latency** needs a game with Reflex, XeLL or AntiLag 2. It's an estimate close to FrameView and PresentMon: expect a few milliseconds of difference, since the overlay can't see the display's flip.

## 📜 Changelog

See [**CHANGELOG-Opti-Venom.md**](CHANGELOG-Opti-Venom.md). Every release says which upstream OptiScaler version it's built on.

## 🙏 Credits

- **[OptiScaler](https://github.com/optiscaler/OptiScaler)** and its team and contributors (cdozdil, FakeMichau, TheRazerMD and everyone else): this build is entirely based on their work. If you like it, [support OptiScaler](https://github.com/sponsors/cdozdil?frequency=one-time).
- **[LorenzoKy](https://github.com/LorenzoKy)**: FPS overlay rework and the OptiScaler Venom builds.

## ☕ Support

OptiScaler Venom is free and always will be. If it's useful to you and you'd like to say thanks, you can [buy me a coffee on Ko-fi](https://ko-fi.com/lorenzoky): completely optional, and appreciated. Supporting the [OptiScaler team](https://github.com/sponsors/cdozdil?frequency=one-time) helps the project this build is made of.

<a href="https://ko-fi.com/lorenzoky"><img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Support me on Ko-fi" /></a>
- The third-party libraries and SDKs OptiScaler uses, listed in the [original README](README-OptiScaler.md).

## ⚖️ License

GPL-3.0, like OptiScaler: see [LICENSE](LICENSE). The full source code of every release is in this repository, tagged with the release's version.
