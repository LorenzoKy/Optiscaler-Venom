#pragma once

#include <Logger.h>
#include <State.h>

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx12.h>

// HDR output for the overlay menu and the FPS overlay on D3D12.
//
// Vertex colors can't encode PQ or values above 1.0 and alpha blending in PQ space is wrong, so on HDR
// swapchains ImGui draws the plain SDR menu into an FP16 layer. A full screen pass then converts that layer
// to the swapchain (PQ Rec.2020 or scRGB) and blends it over a copy of the game image. Blending happens in
// sRGB gamma relative to the UI paper white, so translucent backgrounds and text edges look like in SDR.
//
// Only used for HDR10 (R10G10B10A2 + G2084 P2020) and scRGB (R16G16B16A16_FLOAT + G10 P709) swapchains,
// every other case keeps the regular path.
class MenuHdr_Dx12
{
  public:
    enum class Mode : uint32_t
    {
        None = 0,
        PQ = 1,
        ScRGB = 2
    };

    // Menu colors are left as SDR while this is set, the composite pass converts them
    static bool IsActive() { return _active.load(std::memory_order_relaxed); }

    // Returns true if the state changed
    static bool SetActive(bool active) { return _active.exchange(active, std::memory_order_relaxed) != active; }

    static Mode TargetMode(DXGI_FORMAT format)
    {
        const auto& output = State::Instance().outputColorSpace;

        if (!output.valid || output.model != ColorModel::RGB)
            return Mode::None;

        if (output.transfer == ColorTransfer::PQ && output.primaries == ColorPrimaries::Rec2020 &&
            format == DXGI_FORMAT_R10G10B10A2_UNORM)
        {
            return Mode::PQ;
        }

        if (output.transfer == ColorTransfer::Linear && output.primaries == ColorPrimaries::Rec709 &&
            format == DXGI_FORMAT_R16G16B16A16_FLOAT)
        {
            return Mode::ScRGB;
        }

        return Mode::None;
    }

    // Paper white of the menu in nits, same values the color based tone mapping used
    static float PaperWhite(Mode mode)
    {
        if (mode == Mode::ScRGB && !State::Instance().hdrOutputActive)
            return 80.0f;

        return 203.0f;
    }

    MenuHdr_Dx12(ID3D12Device* device, DescriptorHeapAllocator* srvAlloc) : _device(device), _srvAlloc(srvAlloc)
    {
        _init = CreatePipelines();

        if (!_init)
            LOG_ERROR("HDR menu composite could not be created, using the regular path");
    }

    ~MenuHdr_Dx12()
    {
        ReleaseTextures();

        if (_srvAlloc != nullptr && _srvCpu.ptr != 0)
            _srvAlloc->Free(_srvCpu, _srvGpu);

        // Second descriptor of the pair, allocated separately
        if (_srvAlloc != nullptr && _srvCpu2.ptr != 0)
            _srvAlloc->Free(_srvCpu2, _srvGpu2);

        SafeRelease(_rtvHeap);
        SafeRelease(_layerRootSig);
        SafeRelease(_layerPso);
        SafeRelease(_blurPso);
        SafeRelease(_compositeRootSig);
        SafeRelease(_compositePso);
        SafeRelease(_compositePsoFp16);

        if (_current == this)
            _current = nullptr;
    }

    bool IsInited() const { return _init; }

    // Matches the textures to the back buffer, waitIdle is called before textures still used by the GPU are released
    bool Prepare(ID3D12Resource* backBuffer, Mode mode, const std::function<void()>& waitIdle)
    {
        if (!_init || backBuffer == nullptr || mode == Mode::None)
            return false;

        _mode = mode;

        const auto desc = backBuffer->GetDesc();

        if (_layer != nullptr && _bgCopy != nullptr && desc.Width == _width && desc.Height == _height &&
            desc.Format == _bgFormat)
        {
            return true;
        }

        if (_layer != nullptr || _bgCopy != nullptr)
            waitIdle();

        ReleaseTextures();

        if (!CreateTextures(desc))
        {
            // Don't retry every frame, the regular path takes over
            ReleaseTextures();
            _init = false;
            return false;
        }

        return true;
    }

    // Call after ImGui::Render(), routes the draw data through the HDR pipelines
    void PatchDrawData(ImDrawData* drawData, ImTextureID blurTexture)
    {
        _drawData = drawData;
        _current = this;

        if (drawData == nullptr || drawData->CmdListsCount == 0)
            return;

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            auto& cmds = drawData->CmdLists[n]->CmdBuffer;

            for (int i = 0; i < cmds.Size; i++)
            {
                if (cmds[i].UserCallback == ImDrawCallback_ResetRenderState)
                {
                    // ImGui sets its own pipeline here, switch back to the layer pipeline after it
                    cmds.insert(cmds.Data + i + 1, CallbackCmd(cmds[i], PipelineLayer));
                    i++;
                }
                else if (blurTexture != ImTextureID_Invalid && cmds[i].UserCallback == nullptr &&
                         cmds[i].TexRef._TexData == nullptr && cmds[i].TexRef._TexID == blurTexture)
                {
                    // Blurred game image holds swapchain values, it gets its own conversion
                    cmds.insert(cmds.Data + i, CallbackCmd(cmds[i], PipelineBlur));
                    cmds.insert(cmds.Data + i + 2, CallbackCmd(cmds[i + 1], PipelineLayer));
                    i += 2;
                }
            }
        }

        auto& first = drawData->CmdLists[0]->CmdBuffer;
        if (first.Size > 0)
            first.insert(first.Data, CallbackCmd(first[0], PipelineLayer));
        else
            first.push_back(CallbackCmd(ImDrawCmd(), PipelineLayer));
    }

    // backBuffer must be in COPY_SOURCE state
    void CopyGameImage(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer)
    {
        _cmdList = cmdList;

        TransitionTextures(cmdList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->CopyResource(_bgCopy, backBuffer);
    }

    // Makes the cleared layer the ImGui render target
    void BindLayer(ID3D12GraphicsCommandList* cmdList)
    {
        const float clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        cmdList->ClearRenderTargetView(_layerRtv, clear, 0, nullptr);
        cmdList->OMSetRenderTargets(1, &_layerRtv, FALSE, nullptr);
    }

    // backBufferRtv must be bound-able (back buffer in RENDER_TARGET state), ImGui SRV heap must be set
    void Composite(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv)
    {
        TransitionTextures(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        D3D12_VIEWPORT vp = {};
        vp.Width = (float) _width;
        vp.Height = (float) _height;
        vp.MaxDepth = 1.0f;
        const D3D12_RECT scissor = { 0, 0, (LONG) _width, (LONG) _height };

        const float constants[4] = { (float) _mode, PaperWhite(_mode), 0.0f, 0.0f };

        cmdList->OMSetRenderTargets(1, &backBufferRtv, FALSE, nullptr);
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &scissor);
        cmdList->SetGraphicsRootSignature(_compositeRootSig);
        cmdList->SetPipelineState(_bgFormat == DXGI_FORMAT_R16G16B16A16_FLOAT ? _compositePsoFp16 : _compositePso);
        cmdList->SetGraphicsRoot32BitConstants(0, 4, constants, 0);
        cmdList->SetGraphicsRootDescriptorTable(1, _srvGpu);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        _cmdList = nullptr;
        _drawData = nullptr;
    }

  private:
    static inline std::atomic<bool> _active { false };

    // Only one renderer draws the menu at a time
    static inline MenuHdr_Dx12* _current = nullptr;

    static constexpr intptr_t PipelineLayer = 1;
    static constexpr intptr_t PipelineBlur = 2;

    ID3D12Device* _device = nullptr;
    DescriptorHeapAllocator* _srvAlloc = nullptr;
    bool _init = false;
    Mode _mode = Mode::None;

    ID3D12RootSignature* _layerRootSig = nullptr;
    ID3D12PipelineState* _layerPso = nullptr;
    ID3D12PipelineState* _blurPso = nullptr;
    ID3D12RootSignature* _compositeRootSig = nullptr;
    ID3D12PipelineState* _compositePso = nullptr;     // R10G10B10A2 swapchains
    ID3D12PipelineState* _compositePsoFp16 = nullptr; // R16G16B16A16_FLOAT swapchains

    ID3D12DescriptorHeap* _rtvHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE _layerRtv {};

    // [0] layer SRV, [1] game image copy SRV, need to be contiguous
    D3D12_CPU_DESCRIPTOR_HANDLE _srvCpu {};
    D3D12_GPU_DESCRIPTOR_HANDLE _srvGpu {};
    D3D12_CPU_DESCRIPTOR_HANDLE _srvCpu2 {};
    D3D12_GPU_DESCRIPTOR_HANDLE _srvGpu2 {};

    ID3D12Resource* _layer = nullptr;
    ID3D12Resource* _bgCopy = nullptr;
    D3D12_RESOURCE_STATES _layerState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    D3D12_RESOURCE_STATES _bgState = D3D12_RESOURCE_STATE_COPY_DEST;
    UINT64 _width = 0;
    UINT _height = 0;
    DXGI_FORMAT _bgFormat = DXGI_FORMAT_UNKNOWN;

    ID3D12GraphicsCommandList* _cmdList = nullptr;
    ImDrawData* _drawData = nullptr;

    template <typename T> static void SafeRelease(T*& res)
    {
        if (res != nullptr)
            res->Release();

        res = nullptr;
    }

    void TransitionTextures(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES bgState,
                            D3D12_RESOURCE_STATES layerState)
    {
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        UINT count = 0;

        auto add = [&](ID3D12Resource* res, D3D12_RESOURCE_STATES& current, D3D12_RESOURCE_STATES next)
        {
            if (current == next)
                return;

            auto& barrier = barriers[count++];
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = res;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = current;
            barrier.Transition.StateAfter = next;
            current = next;
        };

        add(_bgCopy, _bgState, bgState);
        add(_layer, _layerState, layerState);

        if (count > 0)
            cmdList->ResourceBarrier(count, barriers);
    }

    static ImDrawCmd CallbackCmd(const ImDrawCmd& source, intptr_t pipeline)
    {
        ImDrawCmd cmd;
        cmd.ClipRect = source.ClipRect;
        cmd.TexRef = source.TexRef;
        cmd.VtxOffset = source.VtxOffset;
        cmd.IdxOffset = source.IdxOffset;
        cmd.ElemCount = 0;
        cmd.UserCallback = &MenuHdr_Dx12::DrawCallback;
        cmd.UserCallbackData = (void*) pipeline;
        return cmd;
    }

    static void DrawCallback(const ImDrawList*, const ImDrawCmd* cmd)
    {
        auto self = _current;
        if (self == nullptr || self->_cmdList == nullptr || self->_drawData == nullptr)
            return;

        auto cmdList = self->_cmdList;
        auto drawData = self->_drawData;

        // Same projection as the ImGui backend
        const float L = drawData->DisplayPos.x;
        const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        const float T = drawData->DisplayPos.y;
        const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        const float mvp[4][4] = {
            { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
            { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.5f, 0.0f },
            { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
        };

        const float constants[4] = { (float) self->_mode, PaperWhite(self->_mode), 0.0f, 0.0f };

        cmdList->SetGraphicsRootSignature(self->_layerRootSig);
        cmdList->SetPipelineState((intptr_t) cmd->UserCallbackData == PipelineBlur ? self->_blurPso : self->_layerPso);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, mvp, 0);
        cmdList->SetGraphicsRoot32BitConstants(2, 4, constants, 0);
    }

    bool CreateTextures(const D3D12_RESOURCE_DESC& backBufferDesc)
    {
        _width = backBufferDesc.Width;
        _height = backBufferDesc.Height;
        _bgFormat = backBufferDesc.Format;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = _width;
        texDesc.Height = _height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.SampleDesc.Count = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

        HRESULT hr =
            _device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&_layer));
        if (FAILED(hr))
        {
            LOG_ERROR("HDR menu layer: {:X}", (unsigned long) hr);
            return false;
        }

        _layer->SetName(L"MenuHdrLayer");
        _layerState = D3D12_RESOURCE_STATE_RENDER_TARGET;

        texDesc.Format = _bgFormat;
        texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        hr = _device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                              D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_bgCopy));
        if (FAILED(hr))
        {
            LOG_ERROR("HDR menu game image copy: {:X}", (unsigned long) hr);
            return false;
        }

        _bgCopy->SetName(L"MenuHdrGameImage");
        _bgState = D3D12_RESOURCE_STATE_COPY_DEST;

        _device->CreateRenderTargetView(_layer, nullptr, _layerRtv);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        _device->CreateShaderResourceView(_layer, &srvDesc, _srvCpu);

        srvDesc.Format = _bgFormat;
        _device->CreateShaderResourceView(_bgCopy, &srvDesc, _srvCpu2);

        LOG_INFO("HDR menu composite: {}x{}, {}, paper white {} nits", _width, _height,
                 _mode == Mode::PQ ? "PQ" : "scRGB", PaperWhite(_mode));

        return true;
    }

    void ReleaseTextures()
    {
        SafeRelease(_layer);
        SafeRelease(_bgCopy);
        _width = 0;
        _height = 0;
        _bgFormat = DXGI_FORMAT_UNKNOWN;
    }

    // The composite table needs two contiguous descriptors from the ImGui heap
    bool AllocateSrvPair()
    {
        struct Handle
        {
            D3D12_CPU_DESCRIPTOR_HANDLE cpu;
            D3D12_GPU_DESCRIPTOR_HANDLE gpu;
        };

        std::vector<Handle> handles;
        const auto step = _srvAlloc->HeapHandleIncrement;
        bool found = false;

        // Take free descriptors until two of them are neighbours, give the others back
        while (!found && _srvAlloc->FreeIndices.Size > 0 && handles.size() < 16)
        {
            Handle h {};
            _srvAlloc->Alloc(&h.cpu, &h.gpu);

            for (const auto& other : handles)
            {
                if (h.cpu.ptr == other.cpu.ptr + step || h.cpu.ptr + step == other.cpu.ptr)
                {
                    const bool otherFirst = other.cpu.ptr < h.cpu.ptr;
                    _srvCpu = otherFirst ? other.cpu : h.cpu;
                    _srvGpu = otherFirst ? other.gpu : h.gpu;
                    _srvCpu2 = otherFirst ? h.cpu : other.cpu;
                    _srvGpu2 = otherFirst ? h.gpu : other.gpu;
                    found = true;
                    break;
                }
            }

            handles.push_back(h);
        }

        for (const auto& h : handles)
        {
            if (!found || (h.cpu.ptr != _srvCpu.ptr && h.cpu.ptr != _srvCpu2.ptr))
                _srvAlloc->Free(h.cpu, h.gpu);
        }

        if (!found)
        {
            _srvCpu = {};
            _srvGpu = {};
            _srvCpu2 = {};
            _srvGpu2 = {};
        }

        return found;
    }

    static ID3DBlob* Compile(const std::string& source, const char* target)
    {
        ID3DBlob* blob = nullptr;
        ID3DBlob* errors = nullptr;

        if (FAILED(D3DCompile(source.c_str(), source.size(), nullptr, nullptr, nullptr, "main", target,
                              D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &blob, &errors)))
        {
            if (errors != nullptr)
                LOG_ERROR("HDR menu shader: {}", (const char*) errors->GetBufferPointer());

            SafeRelease(errors);
            SafeRelease(blob);
            return nullptr;
        }

        SafeRelease(errors);
        return blob;
    }

    bool CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature** rootSig)
    {
        ID3DBlob* blob = nullptr;
        ID3DBlob* errors = nullptr;

        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors);
        SafeRelease(errors);

        if (FAILED(hr))
            return false;

        hr = _device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(rootSig));
        SafeRelease(blob);

        return SUCCEEDED(hr);
    }

    bool CreatePipelines()
    {
        if (_device == nullptr || _srvAlloc == nullptr)
            return false;

        if (!AllocateSrvPair())
        {
            LOG_ERROR("HDR menu: no contiguous descriptors in the menu heap");
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 1;

        if (FAILED(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap))))
            return false;

        _layerRtv = _rtvHeap->GetCPUDescriptorHandleForHeapStart();

        // Layer root signature: ImGui's layout + HDR constants
        {
            D3D12_DESCRIPTOR_RANGE range = {};
            range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            range.NumDescriptors = 1;

            D3D12_ROOT_PARAMETER params[3] = {};
            params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            params[0].Constants.Num32BitValues = 16;
            params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

            params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            params[1].DescriptorTable.NumDescriptorRanges = 1;
            params[1].DescriptorTable.pDescriptorRanges = &range;
            params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            params[2].Constants.ShaderRegister = 1;
            params[2].Constants.Num32BitValues = 4;
            params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_STATIC_SAMPLER_DESC sampler = {};
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            sampler.MaxLOD = D3D12_FLOAT32_MAX;
            sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_ROOT_SIGNATURE_DESC desc = {};
            desc.NumParameters = _countof(params);
            desc.pParameters = params;
            desc.NumStaticSamplers = 1;
            desc.pStaticSamplers = &sampler;
            desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            if (!CreateRootSignature(desc, &_layerRootSig))
                return false;
        }

        // Composite root signature: constants + layer and game image SRVs
        {
            D3D12_DESCRIPTOR_RANGE range = {};
            range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            range.NumDescriptors = 2;

            D3D12_ROOT_PARAMETER params[2] = {};
            params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            params[0].Constants.Num32BitValues = 4;
            params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            params[1].DescriptorTable.NumDescriptorRanges = 1;
            params[1].DescriptorTable.pDescriptorRanges = &range;
            params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_ROOT_SIGNATURE_DESC desc = {};
            desc.NumParameters = _countof(params);
            desc.pParameters = params;

            if (!CreateRootSignature(desc, &_compositeRootSig))
                return false;
        }

        ID3DBlob* layerVs = Compile(LayerVs, "vs_5_0");
        ID3DBlob* layerPs = Compile(LayerPs, "ps_5_0");
        ID3DBlob* blurPs = Compile(std::string(ColorMath) + BlurPs, "ps_5_0");
        ID3DBlob* compositeVs = Compile(CompositeVs, "vs_5_0");
        ID3DBlob* compositePs = Compile(std::string(ColorMath) + CompositePs, "ps_5_0");

        bool result = layerVs && layerPs && blurPs && compositeVs && compositePs;

        if (result)
        {
            static const D3D12_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT) offsetof(ImDrawVert, pos),
                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT) offsetof(ImDrawVert, uv),
                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT) offsetof(ImDrawVert, col),
                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // Same states as the ImGui backend, only the target format differs
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
            pso.pRootSignature = _layerRootSig;
            pso.VS = { layerVs->GetBufferPointer(), layerVs->GetBufferSize() };
            pso.PS = { layerPs->GetBufferPointer(), layerPs->GetBufferSize() };
            pso.InputLayout = { layout, _countof(layout) };
            pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            pso.SampleMask = UINT_MAX;
            pso.NumRenderTargets = 1;
            pso.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            pso.SampleDesc.Count = 1;

            auto& blend = pso.BlendState.RenderTarget[0];
            blend.BlendEnable = TRUE;
            blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            blend.BlendOp = D3D12_BLEND_OP_ADD;
            blend.SrcBlendAlpha = D3D12_BLEND_ONE;
            blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            pso.RasterizerState.DepthClipEnable = TRUE;

            result = SUCCEEDED(_device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&_layerPso)));

            if (result)
            {
                pso.PS = { blurPs->GetBufferPointer(), blurPs->GetBufferSize() };
                result = SUCCEEDED(_device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&_blurPso)));
            }

            if (result)
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC comp = {};
                comp.pRootSignature = _compositeRootSig;
                comp.VS = { compositeVs->GetBufferPointer(), compositeVs->GetBufferSize() };
                comp.PS = { compositePs->GetBufferPointer(), compositePs->GetBufferSize() };
                comp.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                comp.SampleMask = UINT_MAX;
                comp.NumRenderTargets = 1;
                comp.SampleDesc.Count = 1;
                comp.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
                comp.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
                comp.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
                comp.RasterizerState.DepthClipEnable = TRUE;

                // One pipeline per swapchain format
                comp.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
                result = SUCCEEDED(_device->CreateGraphicsPipelineState(&comp, IID_PPV_ARGS(&_compositePso)));

                if (result)
                {
                    comp.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    result = SUCCEEDED(_device->CreateGraphicsPipelineState(&comp, IID_PPV_ARGS(&_compositePsoFp16)));
                }
            }
        }

        SafeRelease(layerVs);
        SafeRelease(layerPs);
        SafeRelease(blurPs);
        SafeRelease(compositeVs);
        SafeRelease(compositePs);

        return result;
    }

    // Shared color math, constants: x = mode (1 PQ, 2 scRGB), y = paper white nits
    static constexpr const char* ColorMath = R"(
static const float3x3 Rec709To2020 = { 0.6274040, 0.3292820, 0.0433136,
                                       0.0690970, 0.9195400, 0.0113612,
                                       0.0163916, 0.0880132, 0.8955950 };
static const float3x3 Rec2020To709 = { 1.6604910, -0.5876411, -0.0728499,
                                      -0.1245505,  1.1328999, -0.0083494,
                                      -0.0181508, -0.1005789,  1.1187297 };

// sRGB curve extended to values above 1 and mirrored for negatives
float3 SrgbToLinear(float3 c)
{
    float3 a = abs(c);
    float3 l = a <= 0.04045 ? a / 12.92 : pow((a + 0.055) / 1.055, 2.4);
    return sign(c) * l;
}

float3 LinearToSrgb(float3 l)
{
    float3 a = abs(l);
    float3 c = a <= 0.0031308 ? a * 12.92 : 1.055 * pow(a, 1.0 / 2.4) - 0.055;
    return sign(l) * c;
}

float3 PqToNits(float3 n)
{
    const float m1 = 2610.0 / 16384.0;
    const float m2 = 2523.0 / 32.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;

    float3 p = pow(saturate(n), 1.0 / m2);
    return 10000.0 * pow(max(p - c1, 0.0) / (c2 - c3 * p), 1.0 / m1);
}

float3 NitsToPq(float3 nits)
{
    const float m1 = 2610.0 / 16384.0;
    const float m2 = 2523.0 / 32.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;

    float3 y = pow(saturate(nits / 10000.0), m1);
    return pow((c1 + c2 * y) / (1.0 + c3 * y), m2);
}

// Swapchain value -> Rec.709 linear relative to paper white
float3 SwapchainToRelative(float3 v, float mode, float paperWhite)
{
    if (mode < 1.5)
        return mul(Rec2020To709, PqToNits(v)) / paperWhite;

    return v * (80.0 / paperWhite);
}

float3 RelativeToSwapchain(float3 rel, float mode, float paperWhite)
{
    if (mode < 1.5)
        return NitsToPq(mul(Rec709To2020, rel * paperWhite));

    return rel * (paperWhite / 80.0);
}
)";

    static constexpr const char* LayerVs = R"(
cbuffer vertexBuffer : register(b0) { float4x4 ProjectionMatrix; };
struct VS_INPUT { float2 pos : POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.0, 1.0));
    output.col = input.col;
    output.uv = input.uv;
    return output;
}
)";

    static constexpr const char* LayerPs = R"(
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);

float4 main(PS_INPUT input) : SV_Target
{
    return input.col * texture0.Sample(sampler0, input.uv);
}
)";

    // Blurred game image -> same space as the SDR layer, the composite turns it back into the game values
    static constexpr const char* BlurPs = R"(
cbuffer hdr : register(b1) { float4 Hdr; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);

float4 main(PS_INPUT input) : SV_Target
{
    float4 texel = texture0.Sample(sampler0, input.uv);
    float3 rel = SwapchainToRelative(texel.rgb, Hdr.x, Hdr.y);
    return input.col * float4(LinearToSrgb(rel), texel.a);
}
)";

    static constexpr const char* CompositeVs = R"(
float4 main(uint id : SV_VertexID) : SV_Position
{
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}
)";

    static constexpr const char* CompositePs = R"(
cbuffer hdr : register(b0) { float4 Hdr; };
Texture2D<float4> layer : register(t0);
Texture2D<float4> game : register(t1);

float4 main(float4 pos : SV_Position) : SV_Target
{
    int3 p = int3(pos.xy, 0);
    float4 ui = layer.Load(p);

    // Leave the game image untouched where there is no menu
    if (ui.a <= 0.0)
        discard;

    float4 bg = game.Load(p);

    // Layer is premultiplied SDR, blend in gamma space like ImGui does on SDR swapchains
    float3 bgGamma = LinearToSrgb(SwapchainToRelative(bg.rgb, Hdr.x, Hdr.y));
    float3 outGamma = ui.rgb + bgGamma * (1.0 - saturate(ui.a));

    return float4(RelativeToSwapchain(SrgbToLinear(outGamma), Hdr.x, Hdr.y), bg.a);
}
)";
};
