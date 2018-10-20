#include "GameObject.h"
#include <filesystem>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std::experimental;

GameObject::GameObject()
{
	XMStoreFloat4x4(&mWorldMatrix, XMMatrixIdentity());
}


DirectX::XMFLOAT3 GameObject::GetPosition() const
{
	return XMFLOAT3(mWorldMatrix(3, 0), mWorldMatrix(3, 1), mWorldMatrix(3, 2));
}



BoundingBox GameObject::GetBoundingBox() const
{
	BoundingBox box;
	mModel.boundingBox.Transform(box, XMLoadFloat4x4(&mWorldMatrix));
	return box;
}

BoundingBox GameObject::GetLocalBoundingBox() const
{
	return mModel.boundingBox;
}

void GameObject::SetModel(Model && model)
{
	std::swap(mModel, model);
	model.modelParts.clear();
	model.boundingBox = BoundingBox();
}

void GameObject::SetModel(const Model & model)
{
	mModel = model;
}

void GameObject::SetWorldMatrix(const XMFLOAT4X4 & world)
{
	mWorldMatrix = world;
}

void GameObject::SetWorldMatrix(FXMMATRIX world)
{
	XMStoreFloat4x4(&mWorldMatrix, world);
}


void GameObject::Draw(ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect)
{

	UINT strides = sizeof(VertexPosNormalTex);
	UINT offsets = 0;

	for (auto& part : mModel.modelParts)
	{
		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 1, part.vertexBuffer.GetAddressOf(), &strides, &offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);
		
		// 更新数据并应用
		effect.SetWorldMatrix(XMLoadFloat4x4(&mWorldMatrix));
		effect.SetTextureAmbient(part.texA);
		effect.SetTextureDiffuse(part.texD);
		effect.SetMaterial(part.material);
		effect.Apply(deviceContext);
		
		deviceContext->DrawIndexed(part.indexCount, 0, 0);
	}
}

