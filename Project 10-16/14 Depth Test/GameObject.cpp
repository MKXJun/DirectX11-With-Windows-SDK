#include "GameObject.h"
#include "d3dUtil.h"
using namespace DirectX;

GameObject::GameObject()
    : m_IndexCount(),
    m_Material(),
    m_VertexStride()
{
}

Transform& GameObject::GetTransform()
{
    return m_Transform;
}

const Transform& GameObject::GetTransform() const
{
    return m_Transform;
}

void GameObject::SetTexture(ID3D11ShaderResourceView* texture)
{
    m_pTexture = texture;
}

void GameObject::SetMaterial(const Material& material)
{
    m_Material = material;
}

void GameObject::Draw(ID3D11DeviceContext * deviceContext, BasicEffect& effect)
{
    // 设置顶点/索引缓冲区
    UINT strides = m_VertexStride;
    UINT offsets = 0;
    deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &strides, &offsets);
    deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    // 更新数据并应用
    effect.SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
    effect.SetTexture(m_pTexture.Get());
    effect.SetMaterial(m_Material);
    effect.Apply(deviceContext);

    deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void GameObject::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
    D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

