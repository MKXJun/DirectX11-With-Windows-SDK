#include "Effects.h"
#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
#include <TextureManager.h>
#include <ModelManager.h>
#include "LightHelper.h"

using namespace DirectX;

# pragma warning(disable: 26812)

//
// BasicEffect::Impl 需要先于BasicEffect的定义
//

class BasicEffect::Impl
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
    D3D11_PRIMITIVE_TOPOLOGY m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;
    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTangentTexLayout;

    bool m_NormalmapEnabled = false;

    XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// BasicEffect
//

namespace
{
    // BasicEffect单例
    static BasicEffect * g_pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
    if (g_pInstance)
        throw std::exception("BasicEffect is a singleton!");
    g_pInstance = this;
    pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
    return *this;
}

BasicEffect & BasicEffect::Get()
{
    if (!g_pInstance)
        throw std::exception("BasicEffect needs an instance!");
    return *g_pInstance;
}


bool BasicEffect::InitAll(ID3D11Device * device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    D3D_SHADER_MACRO defines[] = {
        {"USE_NORMAL_MAP", ""},
        { nullptr, nullptr}
    };

    // ******************
    // 创建顶点着色器
    //

    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"Shaders\\Cache\\");
    
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("BasicVS", L"Shaders\\Basic.hlsl",
        device, "BasicVS", "vs_5_0", nullptr, blob.ReleaseAndGetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTex::GetInputLayout()),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("NormalMapVS", L"Shaders\\Basic.hlsl",
        device, "BasicVS", "vs_5_0", defines, blob.ReleaseAndGetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTangentTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTangentTex::GetInputLayout()),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTangentTexLayout.GetAddressOf()));

    // ******************
    // 创建像素着色器
    //
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("BasicPS", L"Shaders\\Basic.hlsl",
        device, "BasicPS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("NormalMapPS", L"Shaders\\Basic.hlsl",
        device, "BasicPS", "ps_5_0", defines));


    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    passDesc.nameVS = "BasicVS";
    passDesc.namePS = "BasicPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("Basic", device, &passDesc));
    passDesc.nameVS = "NormalMapVS";
    passDesc.namePS = "NormalMapPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("NormalMap", device, &passDesc));

    pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamShadow", RenderStates::SSShadowPCF.Get());

    // 设置调试对象名
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "BasicEffect.VertexPosNormalTexLayout");
    SetDebugObjectName(pImpl->m_pVertexPosNormalTangentTexLayout.Get(), "BasicEffect.VertexPosNormalTangentTexLayout");
#endif
    pImpl->m_pEffectHelper->SetDebugObjectName("BasicEffect");
    
    return true;
}

void BasicEffect::SetMaterial(const Material& material)
{
    TextureManager& tm = TextureManager::Get();

    PhongMaterial phongMat{};
    phongMat.ambient = material.Get<XMFLOAT4>("$AmbientColor");
    phongMat.diffuse = material.Get<XMFLOAT4>("$DiffuseColor");
    phongMat.diffuse.w = material.Has<float>("$Opacity") ? material.Get<float>("$Opacity") : 1.0f;
    phongMat.specular = material.Get<XMFLOAT4>("$SpecularColor");
    phongMat.specular.w = material.Has<float>("$SpecularFactor") ? material.Get<float>("$SpecularFactor") : 1.0f;
    if (material.Has<XMFLOAT4>("$ReflectColor"))
    {
        phongMat.reflect = material.Get<XMFLOAT4>("$ReflectColor");
    }
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Material")->SetRaw(&phongMat);


    auto pStr = material.TryGet<std::string>("$Diffuse");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", pStr ? tm.GetTexture(*pStr) : tm.GetNullTexture());
    pStr = material.TryGet<std::string>("$Normal");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalMap", pStr ? tm.GetTexture(*pStr) : nullptr);
}

MeshDataInput BasicEffect::GetInputData(const MeshData& meshData)
{
    MeshDataInput input;
    input.pInputLayout = pImpl->m_pCurrInputLayout.Get();
    input.topology = pImpl->m_CurrTopology;
    if (pImpl->m_NormalmapEnabled)
    {
        input.pVertexBuffers = {
            meshData.m_pVertices.Get(),
            meshData.m_pNormals.Get(),
            meshData.m_pTangents.Get(),
            meshData.m_pTexcoordArrays.empty() ? nullptr : meshData.m_pTexcoordArrays[0].Get()
        };
        input.strides = { 12, 12, 16, 8 };
        input.offsets = { 0, 0, 0, 0 };
    }
    else
    {
        input.pVertexBuffers = {
            meshData.m_pVertices.Get(),
            meshData.m_pNormals.Get(),
            meshData.m_pTexcoordArrays.empty() ? nullptr : meshData.m_pTexcoordArrays[0].Get(),
            nullptr
        };
        input.strides = { 12, 12, 8, 0 };
        input.offsets = { 0, 0, 0, 0 };
    }

    input.pIndexBuffer = meshData.m_pIndices.Get();
    input.indexCount = meshData.m_IndexCount;

    return input;
}


void BasicEffect::SetRenderDefault()
{
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("Basic");
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTexLayout;
    pImpl->m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    pImpl->m_NormalmapEnabled = false;
}

void BasicEffect::SetRenderWithNormalMap()
{
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("NormalMap");
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTangentTexLayout;
    pImpl->m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    pImpl->m_NormalmapEnabled = true;
}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
    XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV BasicEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
    XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV BasicEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
    XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void XM_CALLCONV BasicEffect::SetShadowTransformMatrix(DirectX::FXMMATRIX S)
{
    XMMATRIX shadowTransform = XMMatrixTranspose(S);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ShadowTransform")->SetFloatMatrix(4, 4, (const float*)&shadowTransform);
}

void BasicEffect::SetDirLight(uint32_t pos, const DirectionalLight & dirLight)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLight")->SetRaw(&dirLight, pos * sizeof(dirLight), sizeof(dirLight));
}

void BasicEffect::SetPointLight(uint32_t pos, const PointLight & pointLight)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PointLight")->SetRaw(&pointLight, pos * sizeof(pointLight), sizeof(pointLight));
}

void BasicEffect::SetSpotLight(uint32_t pos, const SpotLight & spotLight)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SpotLight")->SetRaw(&spotLight, pos * sizeof(spotLight), sizeof(spotLight));
}

void BasicEffect::SetDepthBias(float bias)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DepthBias")->SetFloat(bias);
}

void BasicEffect::SetTextureShadowMap(ID3D11ShaderResourceView* textureShadowMap)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_ShadowMap", textureShadowMap);
}

void BasicEffect::SetTextureCube(ID3D11ShaderResourceView* textureCube)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TexCube", textureCube);
}

void BasicEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EyePosW")->SetFloatVector(3, (FLOAT*)&eyePos);
}

void BasicEffect::Apply(ID3D11DeviceContext * deviceContext)
{
    XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
    XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
    XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

    XMMATRIX VP = V * P;
    XMMATRIX WInvT = XMath::InverseTranspose(W);

    VP = XMMatrixTranspose(VP);
    WInvT = XMMatrixTranspose(WInvT);
    W = XMMatrixTranspose(W);

    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_World")->SetFloatMatrix(4, 4, (FLOAT*)&W);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTranspose")->SetFloatMatrix(4, 4, (FLOAT*)&WInvT);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&VP);

    if (pImpl->m_pCurrEffectPass)
        pImpl->m_pCurrEffectPass->Apply(deviceContext);
}


