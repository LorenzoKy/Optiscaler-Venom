<img src="https://raw.githubusercontent.com/LorenzoKy/Optiscaler-Venom/opti-venom-v0.2-beta1/.github/assets/banner.svg" alt="OptiScaler Venom v0.2 beta 1" width="100%" />

**OptiScaler Venom v0.2 beta 1** by LorenzoKy (LKY). Pre-release: tested on one game, feedback welcome.

> **Built on OptiScaler `10.0.0.1` (development build, after stable `v0.9.4`)**: upstream `master` at [`539c9a1e`](https://github.com/optiscaler/OptiScaler/commit/539c9a1ecd2c37bee1980d0f0645919cab837855), 2026-10-24, same as v0.1.

### ✨ What's new
- **Frame Generation Time** overlay row (D3D12): the GPU time of FSR FG, XeFG or DLSSG per real frame, its average, the time per generated frame and, with FPS Cost, the FPS it takes. Measured live with GPU timestamps, like the upscaler time. Off by default (`FpsOverlayShowFGTime`).
- **Extra dlls**: put more DX12 FFX upscaler dlls (`amd_fidelityfx_upscaler_dx12.dll`, `amd_fidelityfx_dx12.dll`, `amdxcffx64.dll`) in `OptiScaler\Extra dlls` (subfolders allowed). The versions they add show up in the menu's FSR version list next to the default ones, e.g. FSR 4.0.3 next to 4.1.x.
- **Session timer** is now an option of the FPS row instead of the Date & Time row (same `FpsOverlayShowSessionTimer` key).

### 🐛 Fixes
- The overlay no longer resizes and re-centers when a value changes length ([#1](https://github.com/LorenzoKy/Optiscaler-Venom/issues/1)).

### ⚠️ Beta notes
- Tested on Marvel's Spider-Man 2 (RDNA 4, XeFG, FSR 4). FSR FG and DLSSG timing weren't tested. Vulkan frame generation isn't timed.
- Frame generation work running in parallel with the game on another queue counts in full, so the FG time can read a bit high.

Full list in [CHANGELOG-Opti-Venom.md](https://github.com/LorenzoKy/Optiscaler-Venom/blob/opti-venom-v0.2-beta1/CHANGELOG-Opti-Venom.md).

### 📦 Install
Extract `OptiScaler-Venom_v0.2-beta1.7z` next to the game's executable and rename `OptiScaler.dll` as usual (e.g. `dxgi.dll`). Updating? Replace only the DLL and keep your ini. Guide: [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).

### 🙏 Credits
All the upscaling and frame generation work is [OptiScaler](https://github.com/optiscaler/OptiScaler)'s (cdozdil, FakeMichau, TheRazerMD and contributors). OptiScaler Venom and its FPS overlay rework by [LorenzoKy](https://github.com/LorenzoKy) (LKY).

Unofficial build: please report issues [here](https://github.com/LorenzoKy/Optiscaler-Venom/issues), not to the OptiScaler team.
