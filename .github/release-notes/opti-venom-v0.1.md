<img src="https://raw.githubusercontent.com/LorenzoKy/Optiscaler-LKY/opti-venom-v0.1/.github/assets/banner.svg" alt="OptiScaler Venom v0.1" width="100%" />

**First release of OptiScaler Venom** by LorenzoKy (LKY): upstream OptiScaler plus a reworked, fully customizable FPS overlay.

> **Built on OptiScaler `10.0.0.1` (development build, after stable `v0.9.4`)**: upstream `master` at [`539c9a1e`](https://github.com/optiscaler/OptiScaler/commit/539c9a1ecd2c37bee1980d0f0645919cab837855), 2026-10-24. Includes everything in the official nightly of that day.

### ✨ Highlights
- **New metrics**: average, 1% and 0.1% low FPS · PC latency (PresentMon style, experimental) and render latency · bottleneck from GPU usage and CPU wait (GPU / CPU / FPS limit / V-Sync) · real refresh rate / VRR (experimental) · VRAM, GPU, CPU and RAM telemetry with CPU & GPU usage % · date, time and session timer · upscaler with FSR 4 model or DLSS preset.
- **Customization**: any position, scale up to 3x, fonts, colors, outline, rounding, row order, compact labels.
- **Presets**: Opti-Like, Afterburner-Like, Latency Lab. **Share Settings** as a copy/paste code.
- Default settings look exactly like upstream's overlay.

Full list in [CHANGELOG-Opti-Venom.md](https://github.com/LorenzoKy/Optiscaler-LKY/blob/opti-venom-v0.1/CHANGELOG-Opti-Venom.md).

### 📦 Install
Extract `OptiScaler-Venom_v0.1.7z` next to the game's executable and rename `OptiScaler.dll` as usual (e.g. `dxgi.dll`). Updating? Replace only the DLL and keep your ini. Guide: [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).

### 🙏 Credits
All the upscaling and frame generation work is [OptiScaler](https://github.com/optiscaler/OptiScaler)'s (cdozdil, FakeMichau, TheRazerMD and contributors). OptiScaler Venom and its FPS overlay rework by [LorenzoKy](https://github.com/LorenzoKy) (LKY).

Unofficial build: please report issues [here](https://github.com/LorenzoKy/Optiscaler-LKY/issues), not to the OptiScaler team.
