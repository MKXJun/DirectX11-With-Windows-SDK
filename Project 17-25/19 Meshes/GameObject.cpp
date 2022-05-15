#include "GameObject.h"
#include "d3dUtil.h"

using namespace DirectX;

struct InstancedData
{
    XMMATRIX world;
    XMMATRIX worldInvTranspose;
};

Transform& GameObject::GetTransform()
{
    return m_Transform;
}

const Transform& GameObject::GetTransform() const
{
    return m_Transform;
}


BoundingBox GameObject::GetBoundingBox() const
{
    BoundingBox box;
    m_Model.boundingBox.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
    return box;
}

BoundingBox GameObject::GetLocalBoundingBox() const
{
    return m_Model.boundingBox;
}

BoundingOrientedBox GameObject::GetBoundingOrientedBox() const
{
    BoundingOrientedBox box;
    BoundingOrientedBox::CreateFromBoundingBox(box, m_Model.boundingBox);
    box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
    return box;
}

void GameObject::SetModel(Model&& model)
{
    std::swap(m_Model, model);
    model.modelParts.clear();
    model.boundingBox = BoundingBox();
}

void GameObject::SetModel(const Model& model)
{
    m_Model = model;
}

void GameObject::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
    UINT strides = m_Model.vertexStride;
    UINT offsets = 0;

    for (auto& part : m_Model.modelParts)
    {
        // 设置顶点/索引缓冲区
        deviceContext->IASetVertexBuffers(0, 1, part.vertexBuffer.GetAddressOf(), &strides, &offsets);
        deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

        // 更新数据并应用
        effect.SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
        effect.SetTextureDiffuse(part.texDiffuse.Get());
        effect.SetMaterial(part.material);

        effect.Apply(deviceContext);

        deviceContext->DrawIndexed(part.indexCount, 0, 0);
    }
}

void GameObject::SetDebugObjectName(const std::string& name)
{
    m_Model.SetDebugObjectName(name);
}

