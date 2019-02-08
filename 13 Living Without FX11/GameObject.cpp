#include "GameObject.h"
#include "d3dUtil.h"
using namespace DirectX;

GameObject::GameObject()
	: mWorldMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f)
{
}

DirectX::XMFLOAT3 GameObject::GetPosition() const
{
	return XMFLOAT3(mWorldMatrix(3, 0), mWorldMatrix(3, 1), mWorldMatrix(3, 2));
}




void GameObject::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	mTexture = texture;
}

void GameObject::SetMaterial(const Material & material)
{
	mMaterial = material;
}

void GameObject::SetWorldMatrix(const XMFLOAT4X4 & world)
{
	mWorldMatrix = world;
}

void XM_CALLCONV GameObject::SetWorldMatrix(FXMMATRIX world)
{
	XMStoreFloat4x4(&mWorldMatrix, world);
}

void GameObject::Draw(ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect)
{
	// 设置顶点/索引缓冲区
	UINT strides = mVertexStride;
	UINT offsets = 0;
	deviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &strides, &offsets);
	deviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// 更新数据并应用
	effect.SetWorldMatrix(XMLoadFloat4x4(&mWorldMatrix));
	effect.SetTexture(mTexture);
	effect.SetMaterial(mMaterial);
	effect.Apply(deviceContext);

	deviceContext->DrawIndexed(mIndexCount, 0, 0);
}
