#include "Effects.h"
#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
#include <TextureManager.h>

using namespace DirectX;

# pragma warning(disable: 26812)


//
// ForwardEffect::Impl 需要先于ForwardEffect的定义
//

class ForwardEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;
    ComPtr<ID3D11InputLayout> m_pCurrInputLayout;
    D3D11_PRIMITIVE_TOPOLOGY m_Topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;

    XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
    int m_CascadeLevel = 0;
    int m_DerivativeOffset = 0;
    int m_CascadeBlend = 0;
    int m_CascadeSelection = 0;
    int m_PCFKernelSize = 1;
    int m_ShadowSize = 1024;
};

//
// ForwardEffect
//

namespace
{
    // ForwardEffect单例
    static ForwardEffect * g_pInstance = nullptr;
}

ForwardEffect::ForwardEffect()
{
    if (g_pInstance)
        throw std::exception("ForwardEffect is a singleton!");
    g_pInstance = this;
    pImpl = std::make_unique<ForwardEffect::Impl>();
}

ForwardEffect::~ForwardEffect()
{
}

ForwardEffect::ForwardEffect(ForwardEffect && moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
}

ForwardEffect & ForwardEffect::operator=(ForwardEffect && moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
    return *this;
}

ForwardEffect & ForwardEffect::Get()
{
    if (!g_pInstance)
        throw std::exception("ForwardEffect needs an instance!");
    return *g_pInstance;
}


bool ForwardEffect::InitAll(ID3D11Device * device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"Shaders\\Cache");

    // 为了对每个着色器编译出最优版本，需要对同一个文件编译出64种版本的着色器。
    
    const char* numStrs[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8"};
    D3D_SHADER_MACRO defines[] =
    {
        { "CASCADE_COUNT_FLAG", "1" },
        { "USE_DERIVATIVES_FOR_DEPTH_OFFSET_FLAG", "0" },
        { "BLEND_BETWEEN_CASCADE_LAYERS_FLAG", "0" },
        { "SELECT_CASCADE_BY_INTERVAL_FLAG", "0" },
        { nullptr, nullptr }
    };

    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    // 创建顶点着色器
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GeometryVS", L"Shaders/Rendering.hlsl", device,
        "GeometryVS", "vs_5_0", defines, blob.GetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTex::GetInputLayout()),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

    // 前四位代表
    // [级联级别][偏导偏移][级联间混合][级联选择]
    std::string psName = "0000_ForwardPS";
    std::string passName = "0000_Forward";
    EffectPassDesc passDesc;
    
    // 创建通道
    passDesc.nameVS = "GeometryVS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("PreZ_Forward", device, &passDesc));

    for (int cascadeCount = 1; cascadeCount <= 8; ++cascadeCount)
    {
        psName[0] = passName[0] = '0' + cascadeCount;
        defines[0].Definition = numStrs[cascadeCount];
        for (int derivativeIdx = 0; derivativeIdx < 2; ++derivativeIdx)
        {
            psName[1] = passName[1] = '0' + derivativeIdx;
            defines[1].Definition = numStrs[derivativeIdx];
            for (int blendIdx = 0; blendIdx < 2; ++blendIdx)
            {
                psName[2] = passName[2] = '0' + blendIdx;
                defines[2].Definition = numStrs[blendIdx];
                for (int intervalIdx = 0; intervalIdx < 2; ++intervalIdx)
                {
                    psName[3] = passName[3] = '0' + intervalIdx;
                    defines[3].Definition = numStrs[intervalIdx];

                    // 创建像素着色器
                    HR(pImpl->m_pEffectHelper->CreateShaderFromFile(psName, L"Shaders/Rendering.hlsl", device, 
                        "ForwardPS", "ps_5_0", defines));

                    // 创建通道
                    passDesc.nameVS = "GeometryVS";
                    passDesc.namePS = psName;
                    HR(pImpl->m_pEffectHelper->AddEffectPass(passName, device, &passDesc));
                }
            }
        }
    }


    pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSAnistropicWrap16x.Get());
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamShadow", RenderStates::SSShadowPCF.Get());

    // 设置调试对象名
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "ForwardEffect.VertexPosNormalTexLayout");
    pImpl->m_pEffectHelper->SetDebugObjectName("ForwardEffect");
#endif
    

    return true;
}

void XM_CALLCONV ForwardEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
    XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV ForwardEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
    XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV ForwardEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
    XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void ForwardEffect::SetMaterial(const Material& material)
{
    TextureManager& tm = TextureManager::Get();

    auto pStr = material.TryGet<std::string>("$Diffuse");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", pStr ? tm.GetTexture(*pStr) : tm.GetNullTexture());
}

MeshDataInput ForwardEffect::GetInputData(const MeshData& meshData)
{
    MeshDataInput input;
    input.pInputLayout = pImpl->m_pCurrInputLayout.Get();
    input.topology = pImpl->m_Topology;
    input.pVertexBuffers = {
        meshData.m_pVertices.Get(),
        meshData.m_pNormals.Get(),
        meshData.m_pTexcoordArrays.empty() ? nullptr : meshData.m_pTexcoordArrays[0].Get()
    };
    input.strides = { 12, 12, 8 };
    input.offsets = { 0, 0, 0 };
    
    input.pIndexBuffer = meshData.m_pIndices.Get();
    input.indexCount = meshData.m_IndexCount;

    return input;
}

void ForwardEffect::SetCascadeLevels(int cascadeLevels)
{
    pImpl->m_CascadeLevel = cascadeLevels;
}

void ForwardEffect::SetPCFDerivativesOffsetEnabled(bool enable)
{
    pImpl->m_DerivativeOffset = enable;
}

void ForwardEffect::SetCascadeBlendEnabled(bool enable)
{
    pImpl->m_CascadeBlend = enable;
}

void ForwardEffect::SetCascadeIntervalSelectionEnabled(bool enable)
{
    pImpl->m_CascadeSelection = enable;
}

void ForwardEffect::SetCascadeVisulization(bool enable)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VisualizeCascades")->SetSInt(enable);
}

void ForwardEffect::SetCascadeOffsets(const DirectX::XMFLOAT4 offsets[8])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeOffset")->SetRaw(offsets);
}

void ForwardEffect::SetCascadeScales(const DirectX::XMFLOAT4 scales[8])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeScale")->SetRaw(scales);
}

void ForwardEffect::SetCascadeFrustumsEyeSpaceDepths(const float depths[8])
{
    float depthsArray[8][4] = { {depths[0]},{depths[1]}, {depths[2]}, {depths[3]}, 
        {depths[4]}, {depths[5]}, {depths[6]}, {depths[7]} };
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeFrustumsEyeSpaceDepthsFloat")->SetRaw(depths);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeFrustumsEyeSpaceDepthsFloat4")->SetRaw(depthsArray);
}

void ForwardEffect::SetCascadeBlendArea(float blendArea)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeBlendArea")->SetFloat(blendArea);
}

void ForwardEffect::SetPCFKernelSize(int size)
{
    int start = -size / 2;
    int end = size + start;
    pImpl->m_PCFKernelSize = size;
    float padding = (size / 2) / (float)pImpl->m_ShadowSize;
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PCFBlurForLoopStart")->SetSInt(start);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PCFBlurForLoopEnd")->SetSInt(end);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinBorderPadding")->SetFloat(padding);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxBorderPadding")->SetFloat(1.0f - padding);
}

void ForwardEffect::SetPCFDepthOffset(float offset)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ShadowBias")->SetFloat(offset);
}

void ForwardEffect::SetShadowSize(int size)
{
    pImpl->m_ShadowSize = size;
    float padding = (pImpl->m_PCFKernelSize / 2) / (float)size;
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TexelSize")->SetFloat(1.0f / size);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinBorderPadding")->SetFloat(padding);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxBorderPadding")->SetFloat(1.0f - padding);
}

void XM_CALLCONV ForwardEffect::SetShadowViewMatrix(DirectX::FXMMATRIX ShadowView)
{
    XMMATRIX ShadowViewT = XMMatrixTranspose(ShadowView);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ShadowView")->SetFloatMatrix(4, 4, (const float *)&ShadowViewT);
}

void ForwardEffect::SetShadowTextureArray(ID3D11ShaderResourceView* shadow)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_ShadowMap", shadow);
}

void ForwardEffect::SetLightDir(const DirectX::XMFLOAT3& dir)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LightDir")->SetFloatVector(3, (const float*)&dir);
}

void ForwardEffect::SetRenderDefault(bool reversedZ)
{
    std::string passName = "0000_Forward";
    passName[0] = '0' + pImpl->m_CascadeLevel;
    passName[1] = '0' + pImpl->m_DerivativeOffset;
    passName[2] = '0' + pImpl->m_CascadeBlend;
    passName[3] = '0' + pImpl->m_CascadeSelection;
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
    pImpl->m_pCurrEffectPass->SetDepthStencilState(reversedZ ? RenderStates::DSSGreaterEqual.Get() : nullptr, 0);
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTexLayout.Get();
    pImpl->m_Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void ForwardEffect::SetRenderPreZPass(bool reversedZ)
{
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("PreZ_Forward");
    pImpl->m_pCurrEffectPass->SetDepthStencilState(reversedZ ? RenderStates::DSSGreaterEqual.Get() : nullptr, 0);
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTexLayout.Get();
    pImpl->m_Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void ForwardEffect::Apply(ID3D11DeviceContext * deviceContext)
{
    XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
    XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
    XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

    XMMATRIX WV = W * V;
    XMMATRIX WVP = W * V * P;
    XMMATRIX WInvT = XMath::InverseTranspose(W);

    W = XMMatrixTranspose(W);
    WV = XMMatrixTranspose(WV);
    WVP = XMMatrixTranspose(WVP);
    WInvT = XMMatrixTranspose(WInvT);

    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTranspose")->SetFloatMatrix(4, 4, (FLOAT*)&WInvT);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&WVP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldView")->SetFloatMatrix(4, 4, (FLOAT*)&WV);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_World")->SetFloatMatrix(4, 4, (FLOAT*)&W);

    if (pImpl->m_pCurrEffectPass)
        pImpl->m_pCurrEffectPass->Apply(deviceContext);
}


