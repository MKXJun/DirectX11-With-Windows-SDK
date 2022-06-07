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

    struct CBChangesEveryFrame
    {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX worldInvTranspose;
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
        Material material;
        DirectX::XMMATRIX view;
        DirectX::XMFLOAT3 eyePos;
        float cylinderHeight;
    };

public:
    // 必须显式指定
    Impl() : m_IsDirty() {}
    ~Impl() = default;

public:
    // 需要16字节对齐的优先放在前面
    CBufferObject<0, CBChangesEveryFrame>   m_CBFrame;		    // 每次对象绘制的常量缓冲区
    CBufferObject<1, CBChangesOnResize>     m_CBOnResize;		// 每次窗口大小变更的常量缓冲区
    CBufferObject<2, CBChangesRarely>		m_CBRarely;		    // 几乎不会变更的常量缓冲区
    BOOL m_IsDirty;											    // 是否有值变更
    std::vector<CBufferBase*> m_pCBuffers;					    // 统一管理上面所有的常量缓冲区


    ComPtr<ID3D11VertexShader> m_pTriangleVS;
    ComPtr<ID3D11PixelShader> m_pTrianglePS;
    ComPtr<ID3D11GeometryShader> m_pTriangleGS;

    ComPtr<ID3D11VertexShader> m_pCylinderVS;
    ComPtr<ID3D11PixelShader> m_pCylinderPS;
    ComPtr<ID3D11GeometryShader> m_pCylinderGS;

    ComPtr<ID3D11VertexShader> m_pNormalVS;
    ComPtr<ID3D11PixelShader> m_pNormalPS;
    ComPtr<ID3D11GeometryShader> m_pNormalGS;

    ComPtr<ID3D11InputLayout> m_pVertexPosColorLayout;			// VertexPosColor输入布局
    ComPtr<ID3D11InputLayout> m_pVertexPosNormalColorLayout;	// VertexPosNormalColor输入布局

    ComPtr<ID3D11ShaderResourceView> m_pTexture;				// 用于绘制的纹理

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

    // 创建顶点着色器和顶点布局
    HR(CreateShaderFromFile(L"HLSL\\Triangle_VS.cso", L"HLSL\\Triangle_VS.hlsl", "VS", "vs_5_0", blob.GetAddressOf()));
    HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pTriangleVS.GetAddressOf()));
    HR(device->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosColorLayout.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\Cylinder_VS.cso", L"HLSL\\Cylinder_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pCylinderVS.GetAddressOf()));
    HR(device->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalColorLayout.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\Normal_VS.cso", L"HLSL\\Normal_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pNormalVS.GetAddressOf()));

    // 创建像素着色器
    HR(CreateShaderFromFile(L"HLSL\\Triangle_PS.cso", L"HLSL\\Triangle_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pTrianglePS.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\Cylinder_PS.cso", L"HLSL\\Cylinder_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pCylinderPS.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\Normal_PS.cso", L"HLSL\\Normal_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pNormalPS.GetAddressOf()));

    // 创建几何着色器
    HR(CreateShaderFromFile(L"HLSL\\Triangle_GS.cso", L"HLSL\\Triangle_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pTriangleGS.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\Cylinder_GS.cso", L"HLSL\\Cylinder_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pCylinderGS.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\Normal_GS.cso", L"HLSL\\Normal_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pNormalGS.GetAddressOf()));


    pImpl->m_pCBuffers.assign({
        &pImpl->m_CBFrame, 
        &pImpl->m_CBOnResize, 
        &pImpl->m_CBRarely});

    // 创建常量缓冲区
    for (auto& pBuffer : pImpl->m_pCBuffers)
    {
        HR(pBuffer->CreateBuffer(device));
    }

    // 设置调试对象名
    D3D11SetDebugObjectName(pImpl->m_pVertexPosColorLayout.Get(), "VertexPosColorLayout");
    D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalColorLayout.Get(), "VertexPosNormalColorLayout");
    D3D11SetDebugObjectName(pImpl->m_pCBuffers[0]->cBuffer.Get(), "CBFrame");
    D3D11SetDebugObjectName(pImpl->m_pCBuffers[1]->cBuffer.Get(), "CBOnResize");
    D3D11SetDebugObjectName(pImpl->m_pCBuffers[2]->cBuffer.Get(), "CBRarely");
    D3D11SetDebugObjectName(pImpl->m_pTriangleVS.Get(), "Triangle_VS");
    D3D11SetDebugObjectName(pImpl->m_pTriangleGS.Get(), "Triangle_GS");
    D3D11SetDebugObjectName(pImpl->m_pTrianglePS.Get(), "Triangle_PS");
    D3D11SetDebugObjectName(pImpl->m_pCylinderVS.Get(), "Cylinder_VS");
    D3D11SetDebugObjectName(pImpl->m_pCylinderGS.Get(), "Cylinder_GS");
    D3D11SetDebugObjectName(pImpl->m_pCylinderPS.Get(), "Cylinder_PS");
    D3D11SetDebugObjectName(pImpl->m_pNormalVS.Get(), "Normal_VS");
    D3D11SetDebugObjectName(pImpl->m_pNormalGS.Get(), "Normal_GS");
    D3D11SetDebugObjectName(pImpl->m_pNormalPS.Get(), "Normal_PS");


    return true;
}

void BasicEffect::SetRenderSplitedTriangle(ID3D11DeviceContext * deviceContext)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosColorLayout.Get());
    deviceContext->VSSetShader(pImpl->m_pTriangleVS.Get(), nullptr, 0);
    deviceContext->GSSetShader(pImpl->m_pTriangleGS.Get(), nullptr, 0);
    deviceContext->RSSetState(nullptr);
    deviceContext->PSSetShader(pImpl->m_pTrianglePS.Get(), nullptr, 0);

}

void BasicEffect::SetRenderCylinderNoCap(ID3D11DeviceContext * deviceContext)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalColorLayout.Get());
    deviceContext->VSSetShader(pImpl->m_pCylinderVS.Get(), nullptr, 0);
    deviceContext->GSSetShader(pImpl->m_pCylinderGS.Get(), nullptr, 0);
    deviceContext->RSSetState(RenderStates::RSNoCull.Get());
    deviceContext->PSSetShader(pImpl->m_pCylinderPS.Get(), nullptr, 0);

}

void BasicEffect::SetRenderNormal(ID3D11DeviceContext * deviceContext)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalColorLayout.Get());
    deviceContext->VSSetShader(pImpl->m_pNormalVS.Get(), nullptr, 0);
    deviceContext->GSSetShader(pImpl->m_pNormalGS.Get(), nullptr, 0);
    deviceContext->RSSetState(nullptr);
    deviceContext->PSSetShader(pImpl->m_pNormalPS.Get(), nullptr, 0);

}



void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
    auto& cBuffer = pImpl->m_CBFrame;
    cBuffer.data.world = XMMatrixTranspose(W);
    cBuffer.data.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
    auto& cBuffer = pImpl->m_CBRarely;
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
    auto& cBuffer = pImpl->m_CBRarely;
    cBuffer.data.material = material;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
    auto& cBuffer = pImpl->m_CBRarely;
    cBuffer.data.eyePos = eyePos;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetCylinderHeight(float height)
{
    auto& cBuffer = pImpl->m_CBRarely;
    cBuffer.data.cylinderHeight = height;
    pImpl->m_IsDirty = cBuffer.isDirty = true;
}



void BasicEffect::Apply(ID3D11DeviceContext * deviceContext)
{
    auto& pCBuffers = pImpl->m_pCBuffers;
    // 将缓冲区绑定到渲染管线上
    pCBuffers[0]->BindVS(deviceContext);
    pCBuffers[1]->BindVS(deviceContext);
    pCBuffers[2]->BindVS(deviceContext);

    pCBuffers[0]->BindGS(deviceContext);
    pCBuffers[1]->BindGS(deviceContext);
    pCBuffers[2]->BindGS(deviceContext);
    
    pCBuffers[2]->BindPS(deviceContext);

    // 设置纹理
    deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTexture.GetAddressOf());

    if (pImpl->m_IsDirty)
    {
        pImpl->m_IsDirty = false;
        for (auto& pCBuffer : pCBuffers)
        {
            pCBuffer->UpdateBuffer(deviceContext);
        }
    }
}






