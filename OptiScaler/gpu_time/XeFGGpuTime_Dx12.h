#pragma once
#include "SysUtils.h"
#include <d3d12.h>
#include <xefg_swapchain.h>

// GPU time of XeFG measured inside its own command lists. libxess_fg (1.3 and newer) exports
// xefgSwapChainD3D12SetDiagnosticCallbacks, not in the SDK headers: XeFG calls back when it starts and ends recording
// each of its command lists and once per frame. A timestamp goes at both ends of every list and the first list of
// each frame gives the frame generation time, so queue waits and the other lists' copies aren't in it.
// Older libxess_fg versions don't have the export, FGGpuTime_Dx12 times their submissions instead.
namespace XeFGGpuTime_Dx12
{
// Registers the callbacks on the context (again when the context changes), false stops the timing.
// Returns true while the context's lists are timed
bool Enable(xefg_swapchain_handle_t context, bool enable);

// The context is being destroyed, its callbacks are gone with it
void Forget(xefg_swapchain_handle_t context);

// GPU ms of the frames' first lists that finished since the last call
double Collect();
} // namespace XeFGGpuTime_Dx12
