#pragma once

#include "SysUtils.h"

#include "shaders/DA_DAS_Shader.h"
#include "shaders/DA_RCAS_Shader.h"
#include "shaders/NEURAL_Shader.h"
#include "shaders/RCAS_Shader.h"

#include <atomic>

struct RcasConstants
{
    float Sharpness;

    float MvScaleX;
    float MvScaleY;

    float CameraNear;
    float CameraFar;

    bool DepthIsLinear;
    bool DepthIsReversed;

    bool IsHdr;
};

class RCAS_Common
{
  public:
    // The sharpening shader that ran last, its GPU time is shown under this name
    const char* TimedShaderName() const { return _timedShaderName.load(std::memory_order_relaxed); }

  protected:
    // Called by Dispatch with the shader it runs, after the fallbacks
    void SetTimedShader(SharpenShader InShader);

    struct alignas(256) InternalConstants
    {
        float Sharpness;
        float Contrast;

        // Motion Vector Stuff
        int DynamicSharpenEnabled;
        int DisplaySizeMV;
        int Debug;
        int MotionWidth;
        int MotionHeight;

        float MotionSharpness;
        float MotionTextureScale;
        float MvScaleX;
        float MvScaleY;
        float Threshold;
        float ScaleLimit;

        int OutputWidth;
        int OutputHeight;
    };

    struct alignas(256) InternalConstantsDA
    {
        float Sharpness;

        int DepthIsLinear;
        int DepthIsReversed;

        float DepthScale;
        float DepthBias;

        float DepthLinearA;
        float DepthLinearB;
        float DepthLinearC;

        int DynamicSharpenEnabled;
        int DisplaySizeMV;
        int Debug;

        float MotionSharpness;
        float MotionTextureScale;
        float MvScaleX;
        float MvScaleY;
        float MotionThreshold;
        float MotionScaleLimit;

        float DepthTextureScale;

        int ClampOutput;

        int OutputWidth;
        int OutputHeight;
        int MotionWidth;
        int MotionHeight;
        int DepthWidth;
        int DepthHeight;
    };

    // Same order as the cbuffer of neural_sharpen.hlsl
    struct alignas(256) InternalConstantsNeural
    {
        float Sharpness;
        float EdgeResponse;
        int Model;
        int Debug;

        int DynamicSharpenEnabled;
        int DisplaySizeMV;
        int IsHdr;
        int ClampOutput;

        float MotionSharpness;
        float MotionTextureScale;
        float MvScaleX;
        float MvScaleY;

        float MotionThreshold;
        float MotionScaleLimit;
        int OutputWidth;
        int OutputHeight;

        int MotionWidth;
        int MotionHeight;
    };

    // Dx11 and Vulkan write it in the constant buffers of the other shaders
    static_assert(sizeof(InternalConstantsNeural) <= sizeof(InternalConstants));
    static_assert(sizeof(InternalConstantsNeural) <= sizeof(InternalConstantsDA));

    void FillMotionConstants(InternalConstants& OutConstants, const RcasConstants& InConstants);
    void FillMotionConstants(InternalConstantsDA& OutConstants, const RcasConstants& InConstants);
    // Float formats can hold values over 1.0 (linear, before the tonemapper)
    static bool IsFloatFormat(DXGI_FORMAT InFormat);

    // InputFormat is the DXGI or Vulkan format of the sharpened image, only for the log
    void FillMotionConstants(InternalConstantsNeural& OutConstants, const RcasConstants& InConstants, bool InputIsFloat,
                             uint32_t InputFormat);

  private:
    std::atomic<const char*> _timedShaderName { "RCAS" };

    // Last input logged by the neural shader, logged again when it changes
    int64_t _neuralLoggedFormat = -1;
    bool _neuralLoggedLinear = false;
};
