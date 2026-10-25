<img src="https://raw.githubusercontent.com/LorenzoKy/Optiscaler-Venom/opti-venom-v0.2-beta4/.github/assets/banner.svg" alt="OptiScaler Venom v0.2 beta 4" width="100%" />

> **Built on OptiScaler `10.0.0.1` (development build, after stable `v0.9.4`)**: upstream `master` at [`3c6263ec`](https://github.com/optiscaler/OptiScaler/commit/3c6263ec815fd50c09821cbfcf1411d77cb99396), 2026-10-25, with everything in today's official nightly [nightly-20261025](https://github.com/optiscaler/OptiScaler-nightly/releases/tag/nightly-20261025).

### ✨ What's new
- **XeFG in Vulkan games**, by [dashdogy](https://github.com/dashdogy) ([optiscaler/OptiScaler#1240](https://github.com/optiscaler/OptiScaler/pull/1240), not in upstream yet): Vulkan games present through a D3D12 swapchain, so XeFG, XeLL, Dynamic MFG, the FPS limit and the menu work as in D3D12 games. Upscaler and DLSSG (Streamline) FG inputs, HDR included. [MFG Unlocker](https://github.com/dashdogy/MFG-Unlocker)'s XeMFG now works in Vulkan games with Venom too.
- `[XeFG] Vulkan` in `OptiScaler.ini` (on by default): set it to `false` to give Vulkan games their own swapchain back.

### 🐛 Fixes
- From dashdogy's build: the D3D12 menu and FPS overlay no longer hold the present with XeFG.
- XeFG on Vulkan drops a frame it can't present instead of ending in a device lost.

### ⚠️ Notes
- XeFG on Vulkan is brand new: if a Vulkan game breaks with it, set `[XeFG] Vulkan=false` and tell me which game in the [issues](https://github.com/LorenzoKy/Optiscaler-Venom/issues).
- Some drivers may show artifacts in generated frames on Vulkan, and resizing the window while XeFG runs may fail.

Full list in [CHANGELOG-Opti-Venom.md](https://github.com/LorenzoKy/Optiscaler-Venom/blob/opti-venom-v0.2-beta4/CHANGELOG-Opti-Venom.md).

### 📦 Install
Extract `OptiScaler-Venom_v0.2-beta4.7z` next to the game's executable and rename `OptiScaler.dll` as usual (e.g. `dxgi.dll`). Updating? Replace the DLL and take the new `OptiScaler.ini` for the new keys' notes, or keep yours. Guide: [OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki).
