#include "GameObject.h"
#include "d3dUtil.h"
using namespace DirectX;

struct InstancedData
{
	XMMATRIX world;
	XMMATRIX worldInvTranspose;
};

GameObject::GameObject()
	: m_WorldMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f)
{
}

XMFLOAT3 GameObject::GetPosition() const
{
	return XMFLOAT3(m_WorldMatrix(3, 0), m_WorldMatrix(3, 1), m_WorldMatrix(3, 2));
}

BoundingBox GameObject::GetBoundingBox() const
{
	BoundingBox box;
	m_Model.boundingBox.Transform(box, XMLoadFloat4x4(&m_WorldMatrix));
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
	box.Transform(box, XMLoadFloat4x4(&m_WorldMatrix));
	return box;
}

void GameObject::SetModel(Model && model)
{
	std::swap(m_Model, model);
	model.modelParts.clear();
	model.boundingBox = BoundingBox();
}

void GameObject::SetModel(const Model & model)
{
	m_Model = model;
}

void GameObject::SetWorldMatrix(const XMFLOAT4X4 & world)
{
	m_WorldMatrix = world;
}

void XM_CALLCONV GameObject::SetWorldMatrix(FXMMATRIX world)
{
	XMStoreFloat4x4(&m_WorldMatrix, world);
}

void GameObject::Draw(ID3D11DeviceContext * deviceContext, BasicEffect & effect)
{
	UINT strides = m_Model.vertexStride;
	UINT offsets = 0;

	for (auto& part : m_Model.modelParts)
	{
		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 1, part.vertexBuffer.GetAddressOf(), &strides, &offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

		// 更新数据并应用
		effect.SetWorldMatrix(XMLoadFloat4x4(&m_WorldMatrix));
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

