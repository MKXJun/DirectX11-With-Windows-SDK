#include "XUtil.h"
#include "GameObject.h"
#include "DXTrace.h"
#include "ModelManager.h"

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

void GameObject::FrustumCulling(const BoundingFrustum& frustumInWorld)
{
    size_t sz = m_pModel->meshdatas.size();
    m_InFrustum = false;
    m_SubModelInFrustum.resize(sz);
    for (size_t i = 0; i < sz; ++i)
    {
        BoundingOrientedBox box;
        BoundingOrientedBox::CreateFromBoundingBox(box, m_pModel->meshdatas[i].m_BoundingBox);
        box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
        m_SubModelInFrustum[i] = frustumInWorld.Intersects(box);
        m_InFrustum = m_InFrustum || m_SubModelInFrustum[i];
    }
}

void GameObject::CubeCulling(const DirectX::BoundingOrientedBox& obbInWorld)
{
    size_t sz = m_pModel->meshdatas.size();
    m_InFrustum = false;
    m_SubModelInFrustum.resize(sz);
    for (size_t i = 0; i < sz; ++i)
    {
        BoundingOrientedBox box;
        BoundingOrientedBox::CreateFromBoundingBox(box, m_pModel->meshdatas[i].m_BoundingBox);
        box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
        m_SubModelInFrustum[i] = obbInWorld.Intersects(box);
        m_InFrustum = m_InFrustum || m_SubModelInFrustum[i];
    }
}

void GameObject::CubeCulling(const DirectX::BoundingBox& aabbInWorld)
{
    size_t sz = m_pModel->meshdatas.size();
    m_InFrustum = false;
    m_SubModelInFrustum.resize(sz);
    for (size_t i = 0; i < sz; ++i)
    {
        BoundingBox box;
        m_pModel->meshdatas[i].m_BoundingBox.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
        m_SubModelInFrustum[i] = aabbInWorld.Intersects(box);
        m_InFrustum = m_InFrustum || m_SubModelInFrustum[i];
    }
}

void GameObject::SetModel(const Model* pModel)
{
    m_pModel = pModel;
}

const Model* GameObject::GetModel() const
{
    return m_pModel;
}

BoundingBox GameObject::GetLocalBoundingBox() const
{
    return m_pModel ? m_pModel->boundingbox : DirectX::BoundingBox(DirectX::XMFLOAT3(), DirectX::XMFLOAT3());
}

BoundingBox GameObject::GetLocalBoundingBox(size_t idx) const
{
    if (!m_pModel || m_pModel->meshdatas.size() >= idx)
        return DirectX::BoundingBox(DirectX::XMFLOAT3(), DirectX::XMFLOAT3());
    return m_pModel->meshdatas[idx].m_BoundingBox;
}

BoundingBox GameObject::GetBoundingBox() const
{
    if (!m_pModel)
        return DirectX::BoundingBox(DirectX::XMFLOAT3(), DirectX::XMFLOAT3());
    BoundingBox box = m_pModel->boundingbox;
    box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
    return box;
}

BoundingBox GameObject::GetBoundingBox(size_t idx) const
{
    if (!m_pModel || m_pModel->meshdatas.size() >= idx)
        return DirectX::BoundingBox(DirectX::XMFLOAT3(), DirectX::XMFLOAT3());
    BoundingBox box = m_pModel->meshdatas[idx].m_BoundingBox;
    box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
    return box;
}

BoundingOrientedBox GameObject::GetBoundingOrientedBox() const
{
    if (!m_pModel)
        return DirectX::BoundingOrientedBox(DirectX::XMFLOAT3(), DirectX::XMFLOAT3(), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    BoundingOrientedBox obb;
    BoundingOrientedBox::CreateFromBoundingBox(obb, m_pModel->boundingbox);
    obb.Transform(obb, m_Transform.GetLocalToWorldMatrixXM());
    return obb;
}
BoundingOrientedBox GameObject::GetBoundingOrientedBox(size_t idx) const
{
    if (!m_pModel || m_pModel->meshdatas.size() >= idx)
        return DirectX::BoundingOrientedBox(DirectX::XMFLOAT3(), DirectX::XMFLOAT3(), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    BoundingOrientedBox obb;
    BoundingOrientedBox::CreateFromBoundingBox(obb, m_pModel->meshdatas[idx].m_BoundingBox);
    obb.Transform(obb, m_Transform.GetLocalToWorldMatrixXM());
    return obb;
}

void GameObject::Draw(ID3D11DeviceContext * deviceContext, IEffect& effect)
{
    if (!m_InFrustum || !deviceContext)
        return;
    size_t sz = m_pModel->meshdatas.size();
    size_t fsz = m_SubModelInFrustum.size();
    for (size_t i = 0; i < sz; ++i)
    {
        if (i < fsz && !m_SubModelInFrustum[i])
            continue;

        IEffectMeshData* pEffectMeshData = dynamic_cast<IEffectMeshData*>(&effect);
        if (!pEffectMeshData)
            continue;

        IEffectMaterial* pEffectMaterial = dynamic_cast<IEffectMaterial*>(&effect);
        if (pEffectMaterial)
            pEffectMaterial->SetMaterial(m_pModel->materials[m_pModel->meshdatas[i].m_MaterialIndex]);

        IEffectTransform* pEffectTransform = dynamic_cast<IEffectTransform*>(&effect);
        if (pEffectTransform)
            pEffectTransform->SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());

        effect.Apply(deviceContext);

        MeshDataInput input = pEffectMeshData->GetInputData(m_pModel->meshdatas[i]);
        {
            deviceContext->IASetInputLayout(input.pInputLayout);
            deviceContext->IASetPrimitiveTopology(input.topology);
            deviceContext->IASetVertexBuffers(0, (uint32_t)input.pVertexBuffers.size(), 
                input.pVertexBuffers.data(), input.strides.data(), input.offsets.data());
            deviceContext->IASetIndexBuffer(input.pIndexBuffer, input.indexCount > 65535 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);

            deviceContext->DrawIndexed(input.indexCount, 0, 0);
        }
        
    }
}


