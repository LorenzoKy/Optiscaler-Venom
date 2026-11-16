#include "pch.h"

#include "RCAS_Common.h"

void RCAS_Common::FillMotionConstants(InternalConstants& OutConstants, const RcasConstants& InConstants)
{
    if (Config::Instance()->ContrastEnabled.value_or_default())
        OutConstants.Contrast = Config::Instance()->Contrast.value_or_default();
    else
        OutConstants.Contrast = 0.0f;

    OutConstants.Sharpness = InConstants.Sharpness;
    OutConstants.MvScaleX = InConstants.MvScaleX;
    OutConstants.MvScaleY = InConstants.MvScaleY;

    OutConstants.DisplaySizeMV = OutConstants.MotionWidth == OutConstants.OutputWidth ? 1 : 0;

    OutConstants.DynamicSharpenEnabled = Config::Instance()->MotionSharpnessEnabled.value_or_default() ? 1 : 0;
    OutConstants.MotionSharpness = Config::Instance()->MotionSharpness.value_or_default();
    OutConstants.Debug = Config::Instance()->MotionSharpnessDebug.value_or_default() ? 1 : 0;
    OutConstants.Threshold = Config::Instance()->MotionThreshold.value_or_default();
    OutConstants.ScaleLimit = Config::Instance()->MotionScaleLimit.value_or_default();

    if (OutConstants.DisplaySizeMV)
        OutConstants.MotionTextureScale = 1.0f;
    else
        OutConstants.MotionTextureScale = (float) OutConstants.MotionWidth / (float) OutConstants.OutputWidth;
}

void RCAS_Common::SetTimedShader(SharpenShader InShader)
{
    // Same names as the Method combo of the menu
    const char* name = "RCAS";

    switch (InShader)
    {
    case SharpenShader::DepthAware:
        name = "Depth Aware (RCAS)";
        break;
    case SharpenShader::LocalContrastDepthAware:
        name = "Depth Aware (DAS)";
        break;
    case SharpenShader::Neural:
        name = "Neural";
        break;
    default:
        break;
    }

    _timedShaderName.store(name, std::memory_order_relaxed);
}

bool RCAS_Common::IsFloatFormat(DXGI_FORMAT InFormat)
{
    switch (InFormat)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return true;
    default:
        return false;
    }
}

void RCAS_Common::FillMotionConstants(InternalConstantsNeural& OutConstants, const RcasConstants& InConstants,
                                      bool InputIsFloat, uint32_t InputFormat)
{
    // Like the depth aware shaders, 0.5 is the default Intensity (1.0) of the ReShade shader
    OutConstants.Sharpness = InConstants.Sharpness * 2.0f;
    OutConstants.EdgeResponse =
        std::clamp(Config::Instance()->NeuralSharpenEdgeResponse.value_or_default(), -1.0f, 1.0f);
    OutConstants.Model = std::clamp(Config::Instance()->NeuralSharpenModel.value_or_default(), 0, 1);

    // HDR inputs are linear and go over 1.0. Games also send float images before their tonemapper without the
    // HDR flag, so those are handled like HDR too: clamping them to 1.0 would cut every highlight. Only unorm
    // images are display ready and stay in [0, 1]
    const bool linear = InConstants.IsHdr || InputIsFloat;
    OutConstants.IsHdr = linear ? 1 : 0;
    OutConstants.ClampOutput = linear ? 0 : 1;

    if (_neuralLoggedFormat != (int64_t) InputFormat || _neuralLoggedLinear != linear)
    {
        _neuralLoggedFormat = InputFormat;
        _neuralLoggedLinear = linear;
        LOG_INFO("Neural sharpening input format: {}, float: {}, HDR flag: {}, handled as {}", InputFormat,
                 InputIsFloat, InConstants.IsHdr, linear ? "linear" : "display ready");
    }

    OutConstants.MvScaleX = InConstants.MvScaleX;
    OutConstants.MvScaleY = InConstants.MvScaleY;

    OutConstants.DynamicSharpenEnabled = Config::Instance()->MotionSharpnessEnabled.value_or_default() ? 1 : 0;
    OutConstants.Debug = Config::Instance()->MotionSharpnessDebug.value_or_default() ? 1 : 0;
    OutConstants.MotionSharpness = Config::Instance()->MotionSharpness.value_or_default();
    OutConstants.MotionThreshold = Config::Instance()->MotionThreshold.value_or_default();
    OutConstants.MotionScaleLimit = Config::Instance()->MotionScaleLimit.value_or_default();

    OutConstants.DisplaySizeMV = OutConstants.MotionWidth == OutConstants.OutputWidth ? 1 : 0;

    if (OutConstants.DisplaySizeMV)
        OutConstants.MotionTextureScale = 1.0f;
    else
        OutConstants.MotionTextureScale = (float) OutConstants.MotionWidth / (float) OutConstants.OutputWidth;
}

void RCAS_Common::FillMotionConstants(InternalConstantsDA& OutConstants, const RcasConstants& InConstants)
{
    OutConstants.Sharpness = InConstants.Sharpness * 2.0f;
    OutConstants.MvScaleX = InConstants.MvScaleX;
    OutConstants.MvScaleY = InConstants.MvScaleY;

    OutConstants.DynamicSharpenEnabled = Config::Instance()->MotionSharpnessEnabled.value_or_default() ? 1 : 0;
    OutConstants.Debug = Config::Instance()->MotionSharpnessDebug.value_or_default() ? 3 : 0;
    OutConstants.MotionSharpness = Config::Instance()->MotionSharpness.value_or_default();
    OutConstants.MotionThreshold = Config::Instance()->MotionThreshold.value_or_default();
    OutConstants.MotionScaleLimit = Config::Instance()->MotionScaleLimit.value_or_default();

    OutConstants.DepthIsLinear = InConstants.DepthIsLinear ? 1 : 0;
    OutConstants.DepthIsReversed = InConstants.DepthIsReversed ? 1 : 0;
    OutConstants.DepthScale =
        Config::Instance()->DADepthScale.value_or(OutConstants.DepthIsLinear == 0 ? 35.0f : 250.0f);
    OutConstants.DepthBias =
        Config::Instance()->DADepthBias.value_or(OutConstants.DepthIsLinear == 0 ? 0.001f : 0.0015f);

    OutConstants.DepthLinearA = InConstants.CameraNear * InConstants.CameraFar;
    OutConstants.DepthLinearB = InConstants.CameraFar;
    OutConstants.DepthLinearC = InConstants.CameraFar - InConstants.CameraNear;

    OutConstants.DepthTextureScale = (float) OutConstants.DepthWidth / (float) OutConstants.OutputWidth;
    OutConstants.ClampOutput = Config::Instance()->DAClampOutput.value_or(InConstants.IsHdr) ? 0 : 1;

    OutConstants.DisplaySizeMV = OutConstants.MotionWidth == OutConstants.OutputWidth ? 1 : 0;

    if (OutConstants.DisplaySizeMV)
        OutConstants.MotionTextureScale = 1.0f;
    else
        OutConstants.MotionTextureScale = (float) OutConstants.MotionWidth / (float) OutConstants.OutputWidth;
}
