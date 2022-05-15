#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;

//
// BasicEffect::Impl 需要先于BasicEffect的定义
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:

    //
    // 这些结构体对应HLSL的结构体。需要按16字节对齐
    //

    struct CBChangesEveryDrawing
    {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX worldInvTranspose;
        Material material;
    };

    struct CBChangesEveryFrame
    {
        DirectX::XMMATRIX view;
        DirectX::XMFLOAT3 eyePos;
        float pad;
    };

    struct CBDrawingStates
    {
        DirectX::XMVECTOR fogColor;
        int fogEnabled;
        float fogStart;
        float fogRange;
        float pad;
    };

    struct CBChangesOnResize
    {
        DirectX::XMMATRIX proj;
    };


    struct CBChangesRarely
    {
        DirectionalLight dirLight[BasicEffect::maxLights];
        PointLight pointLight[BasicEffect::maxLights];
        SpotLight spotLight[BasicEffect::maxLights];
    };

public:
    // 必须显式指定
    Impl() : m_IsDirty() {}
    ~Impl() = default;

public:
    // 需要16字节对齐的优先放在前面
    CBufferObject<0, CBChangesEveryDrawing> m_CBDrawing;		// 每次对象绘制的常量缓冲区
    CBufferObject<1, CBChangesEveryFrame>   m_CBFrame;		    // 每帧绘制的常量缓冲区
    CBufferObject<2, CBDrawingStates>       m_CBStates;		    // 每次绘制状态变更的常量缓冲区
    CBufferObject<3, CBChangesOnResize>     m_CBOnResize;		// 每次窗口大小变更的常量缓冲区
    CBufferObject<4, CBChangesRarely>		m_CBRarely;		    // 几乎不会变更的常量缓冲区
    BOOL m_IsDirty;											    // 是否有值变更
    std::vector<CBufferBase*> m_pCBuffers;					    // 统一管理上面所有的常量缓冲区


    ComPtr<ID3D11VertexShader> m_pBasicVS;
    ComPtr<ID3D11PixelShader> m_pBasicPS;

    ComPtr<ID3D11VertexShader> m_pBillboardVS;
    ComPtr<ID3D11GeometryShader> m_pBillboardGS;
    ComPtr<ID3D11PixelShader> m_pBillboardPS;


    ComPtr<ID3D11InputLayout> m_pVertexPosSizeLayout;			// 点精灵输入布局
    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;		// 3D顶点输入布局

    ComPtr<ID3D11ShaderResourceView> m_pTexture;				// 用于绘制的纹理
    ComPtr<ID3D11ShaderResourceView> m_pTextures;				// 用于绘制的纹理数组
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

    if (!pImpl->m_pCBuffers.empty())
        return true;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    ComPtr<ID3DBlob> blob;

    // ******************
    // 常规3D绘制
    //
    HR(CreateShaderFromFile(L"HLSL\\Basic_VS.cso", L"HLSL\\Basic_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicVS.GetAddressOf()));
    // 创建顶点输入布局
    HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout), blob->GetBufferPointer(),
        blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));
    HR(CreateShaderFromFile(L"HLSL\\Basic_PS.cso", L"HLSL\\Basic_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicPS.GetAddressOf()));


    // ******************
    // 绘制公告板
    //
    HR(CreateShaderFromFile(L"HLSL\\Billboard_VS.cso", L"HLSL\\Billboard_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBillboardVS.GetAddressOf()));
    // 创建顶点输入布局
    HR(device->CreateInputLayout(VertexPosSize::inputLayout, ARRAYSIZE(VertexPosSize::inputLayout), blob->GetBufferPointer(),
        blob->GetBufferSize(), pImpl->m_pVertexPosSizeLayout.GetAddressOf()));
    HR(CreateShaderFromFile(L"HLSL\\Billboard_GS.cso", L"HLSL\\Billboard_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBillboardGS.GetAddressOf()));
    HR(CreateShaderFromFile(L"HLSL\\Billboard_PS.cso", L"HLSL\\Billboard_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBillboardPS.GetAddressOf()));


    pImpl->m_pCBuffers.assign({
        &pImpl->m_CBDrawing, 
        &pImpl->m_CBFrame, 
        &pImpl->m_CBStates, 
        &pImpl->m_CBOnResize, 
        &pImpl->m_CBRarely});

    // 创建常量缓冲区
    for (auto& pBuffer : pImpl->m_pCBuffers)
    {
        HR(pBuffer->CreateBuffer(device));
    }

    // 设置调试对象名
    D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "VertexPosNormalTexLayout");
    D3D11SetDebugObjectName(pImpl->m_pVertexPosSizeLayout.Get(), "VertexPosSizeLayout");
    D3D11SetDebugObjectName(pImpl->m_pCBuffers[0]->cBuffer.Get(), "CBDrawing");
    D3D11SetDebugObjectName(pImpl->m_pCBuffers[1]->cBuffer.Get(), "CBFrame");
    D3D11SetDebugObjectName(pImpl->m_pCBuffers[2]->cBuffer.Get(), "CBStates");
    D3D11SetDebugObjectName(pImpl->m_pCBuffers[3]->cBuffer.Get(), "CBOnResize");
    D3D11SetDebugObjectName(pImpl->m_pCBuffers[4]->cBuffer.Get(), "CBRarely");
    D3D11SetDebugObjectName(pImpl->m_pBasicVS.Get(), "Basic_VS");
    D3D11SetDebugObjectName(pImpl->m_pBasicPS.Get(), "Basic_PS");
    D3D11SetDebugObjectName(pImpl->m_pBillboardVS.Get(), "Billboard_VS");
    D3D11SetDebugObjectName(pImpl->m_pBillboardGS.Get(), "Billboard_GS");
    D3D11SetDebugObjectName(pImpl->m_pBillboardPS.Get(), "Billboard_PS");

    return true;
}

void BasicEffect::SetRenderDefault(ID3D11DeviceContext * deviceContext)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
    deviceContext->VSSetShader(pImpl->m_pBasicVS.Get(), nullptr, 0);
    deviceContext->GSSetShader(nullptr, nullptr, 0);
    deviceContext->RSSetState(nullptr);
    deviceContext->PSSetShader(pImpl->m_pBasicPS.Get(), nullptr, 0);
    deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
    deviceContext->OMSetDepthStencilState(nullptr, 0);
    deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicEffect::SetRenderBillboard(ID3D11DeviceContext * deviceContext, bool enableAlphaToCoverage)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosSizeLayout.Get());
    deviceContext->VSSetShader(pImpl->m_pBillboardVS.Get(), nullptr, 0);
    deviceContext->GSSetShader(pImpl->m_pBillboardGS.Get(), nullptr, 0);
    deviceContext->RSSetState(RenderStates::RSNoCull.Get());
    deviceContext->PSSetShader(pImpl->m_pBillboardPS.Get(), nullptr, 0);
    deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
    deviceContext->OMSetDepthStencilState(nullptr, 0);
    deviceContext->OMSetBlendState(
        (enableAlphaToCoverage ? RenderStates::BSAlphaToCoverage.Get() : nullptr),
        nullptr, 0xFFFFFFFF);

}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
    auto& cBuffer = pImpl->m_CBDrawing;
    cBuffer.data.world = XMMatrixTranspose(W);
    cBuffer.data.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
    auto& cBuffer = pImpl->m_CBFrame;
    cBuffer.data.view = XMMatrixTranspose(V);
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
    auto& cBuffer = pImpl->m_CBOnResize;
    cBuffer.data.proj = XMMatrixTranspose(P);
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
    auto& cBuffer = pImpl->m_CBRarely;
    cBuffer.data.dirLight[pos] = dirLight;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
    auto& cBuffer = pImpl->m_CBRarely;
    cBuffer.data.pointLight[pos] = pointLight;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
    auto& cBuffer = pImpl->m_CBRarely;
    cBuffer.data.spotLight[pos] = spotLight;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material & material)
{
    auto& cBuffer = pImpl->m_CBDrawing;
    cBuffer.data.material = material;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTexture(ID3D11ShaderResourceView * texture)
{
    pImpl->m_pTexture = texture;
}

void BasicEffect::SetTextureArray(ID3D11ShaderResourceView * textures)
{
    pImpl->m_pTextures = textures;
}

void BasicEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
    auto& cBuffer = pImpl->m_CBFrame;
    cBuffer.data.eyePos = eyePos;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}



void BasicEffect::SetFogState(bool isOn)
{
    auto& cBuffer = pImpl->m_CBStates;
    cBuffer.data.fogEnabled = isOn;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogStart(float fogStart)
{
    auto& cBuffer = pImpl->m_CBStates;
    cBuffer.data.fogStart = fogStart;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogColor(DirectX::XMVECTOR fogColor)
{
    auto& cBuffer = pImpl->m_CBStates;
    cBuffer.data.fogColor = fogColor;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogRange(float fogRange)
{
    auto& cBuffer = pImpl->m_CBStates;
    cBuffer.data.fogRange = fogRange;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ID3D11DeviceContext * deviceContext)
{
    auto& pCBuffers = pImpl->m_pCBuffers;
    // 将缓冲区绑定到渲染管线上
    pCBuffers[0]->BindVS(deviceContext);
    pCBuffers[1]->BindVS(deviceContext);
    pCBuffers[3]->BindVS(deviceContext);

    pCBuffers[0]->BindGS(deviceContext);
    pCBuffers[1]->BindGS(deviceContext);
    pCBuffers[3]->BindGS(deviceContext);

    pCBuffers[0]->BindPS(deviceContext);
    pCBuffers[1]->BindPS(deviceContext);
    pCBuffers[2]->BindPS(deviceContext);
    pCBuffers[4]->BindPS(deviceContext);

    // 设置纹理
    deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTexture.GetAddressOf());
    deviceContext->PSSetShaderResources(1, 1, pImpl->m_pTextures.GetAddressOf());

    if (pImpl->m_IsDirty)
    {
        pImpl->m_IsDirty = false;
        for (auto& pCBuffer : pCBuffers)
        {
            pCBuffer->UpdateBuffer(deviceContext);
        }
    }
}

