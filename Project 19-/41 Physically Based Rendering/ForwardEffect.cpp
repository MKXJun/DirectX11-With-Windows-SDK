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

    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTangentTexLayout;

    XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
    int m_ShadowType = 0;
    int m_CascadeLevel = 0;
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

    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"Shaders\\Cache", true);

    // 为了对每个着色器编译出最优版本，需要对同一个文件编译出64种版本的着色器。
    
    const char* numStrs[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8"};
    D3D_SHADER_MACRO defines[] =
    {
        { "SHADOW_TYPE", "0" },
        { "CASCADE_COUNT_FLAG", "1" },
        { "BLEND_BETWEEN_CASCADE_LAYERS_FLAG", "0" },
        { "SELECT_CASCADE_BY_INTERVAL_FLAG", "0" },
        { nullptr, nullptr }
    };

    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    // 创建顶点着色器
    pImpl->m_pEffectHelper->CreateShaderFromFile("GeometryVS", L"Shaders/Rendering.hlsl", device,
        "GeometryVS", "vs_5_0", defines, blob.GetAddressOf());
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTangentTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTangentTex::GetInputLayout()),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTangentTexLayout.GetAddressOf()));


    // 创建像素着色器
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ForwardPS", L"Shaders/Rendering.hlsl", device,
        "ForwardPS", "ps_5_0"));

    // 创建通道
    EffectPassDesc passDesc;
    passDesc.nameVS = "GeometryVS";
    passDesc.namePS = "ForwardPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("Forward", device, &passDesc));
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamplerDiffuse", RenderStates::SSLinearWrap.Get());

    // 设置调试对象名
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(pImpl->m_pVertexPosNormalTangentTexLayout.Get(), "ForwardEffect.VertexPosNormalTangentTexLayout");
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

    auto pStr = material.TryGet<std::string>("$Albedo");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_AlbedoMap", pStr ? tm.GetTexture(*pStr) : nullptr);
    pStr = material.TryGet<std::string>("$Normal");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalMap", pStr ? tm.GetTexture(*pStr) : nullptr);
    // Roughness, Metalness, AO合并为一张图
    pStr = material.TryGet<std::string>("$Roughness");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_RMAMap", pStr ? tm.GetTexture(*pStr) : nullptr);

}

MeshDataInput ForwardEffect::GetInputData(const MeshData& meshData)
{
    MeshDataInput input;
    input.pInputLayout = pImpl->m_pCurrInputLayout.Get();
    input.topology = pImpl->m_Topology;
    input.pVertexBuffers = {
        meshData.m_pVertices.Get(),
        meshData.m_pNormals.Get(),
        meshData.m_pTangents.Get(),
        meshData.m_pTexcoordArrays.empty() ? nullptr : meshData.m_pTexcoordArrays[0].Get()
    };
    input.strides = { 12, 12, 16, 8 };
    input.offsets = { 0, 0, 0, 0 };
    
    input.pIndexBuffer = meshData.m_pIndices.Get();
    input.indexCount = meshData.m_IndexCount;

    return input;
}

void ForwardEffect::SetShadowType(int type)
{
    if (type > 4 || type < 0)
        return;
    pImpl->m_ShadowType = type;
}

void ForwardEffect::SetCascadeLevels(int cascadeLevels)
{
    pImpl->m_CascadeLevel = cascadeLevels;
}

void ForwardEffect::SetCascadeIntervalSelectionEnabled(bool enable)
{
    pImpl->m_CascadeSelection = enable;
}

void ForwardEffect::SetCascadeVisulization(bool enable)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VisualizeCascades")->SetSInt(enable);
}

void ForwardEffect::Set16BitFormatShadow(bool enable)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_16BitShadow")->SetSInt(enable);
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
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeFrustumsEyeSpaceDepthsData")->SetRaw(depths);
}

void ForwardEffect::SetCascadeBlendArea(float blendArea)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeBlendArea")->SetFloat(blendArea);
}

void ForwardEffect::SetMagicPower(float power)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MagicPower")->SetFloat(power);
}

void ForwardEffect::SetCameraPos(const DirectX::XMFLOAT3& pos)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CameraPosW")->SetFloatVector(3, reinterpret_cast<const float*>(&pos));
}

void ForwardEffect::SetDirectionalLights(uint32_t numLights, const DirectX::XMFLOAT4 lightDirs[4], const DirectX::XMFLOAT4 lightColors[4])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_NumLights")->SetUInt(numLights);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LightDirections")->SetRaw(lightDirs);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LightColors")->SetRaw(lightColors);
}

void ForwardEffect::SetShadowSize(int size)
{
    pImpl->m_ShadowSize = size;
    float padding = 1.0f / (float)size;
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
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureShadow", shadow);
}

void ForwardEffect::SetPosExponent(float posExp)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EvsmPosExp")->SetFloat(posExp);
}

void ForwardEffect::SetNegExponent(float negExp)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EvsmNegExp")->SetFloat(negExp);
}

void ForwardEffect::SetLightBleedingReduction(float value)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LightBleedingReduction")->SetFloat(value);
}

void ForwardEffect::SetCascadeSampler(ID3D11SamplerState* sampler)
{
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamplerShadow", sampler);
}

void ForwardEffect::SetPCFKernelSize(int size)
{
    int start = -size / 2;
    int end = size + start;
    pImpl->m_PCFKernelSize = size;
    float padding = (float)(size / 2) / (float)pImpl->m_ShadowSize;
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PCFBlurForLoopStart")->SetSInt(start);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PCFBlurForLoopEnd")->SetSInt(end);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinBorderPadding")->SetFloat(padding);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxBorderPadding")->SetFloat(1.0f - padding);
}

void ForwardEffect::SetPCFDepthBias(float offset)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PCFDepthBias")->SetFloat(offset);
}

void ForwardEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext, bool reversedZ)
{
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("Forward");
    pImpl->m_pCurrEffectPass->SetDepthStencilState(reversedZ ? RenderStates::DSSGreaterEqual.Get() : nullptr, 0);
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTangentTexLayout.Get();
    pImpl->m_Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void ForwardEffect::SetRenderPreZPass(ID3D11DeviceContext* deviceContext, bool reversedZ)
{
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


