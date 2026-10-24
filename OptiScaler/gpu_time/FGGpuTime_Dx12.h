#pragma once
#include "SysUtils.h"
#include <d3d12.h>

// GPU time of the frame generation's own work, for every output (FSR FG, XeFG, DLSSG) without hooks into their SDKs.
// The ExecuteCommandLists calls made from the frame generation's libraries get timestamps around them, on whatever
// queue they submit to. That's the GPU time of the interpolation and of the frame generation swapchain's copies and
// composition. It's wall time on each queue: work that overlaps the game's on another queue counts in full.
// Opti's own lists for the frame generation (resource copies, UI) aren't in it, they're submitted by Opti.
namespace FGGpuTime_Dx12
{
// Hooks ExecuteCommandLists of the queue and of the direct and compute queues of its device, each function once
void Hook(ID3D12CommandQueue* queue);

// Libraries whose submissions are timed, none stops the timing
void SetModules(const std::vector<HMODULE>& modules);

// GPU ms of the timed submissions that finished since the last call
double Collect();
} // namespace FGGpuTime_Dx12
