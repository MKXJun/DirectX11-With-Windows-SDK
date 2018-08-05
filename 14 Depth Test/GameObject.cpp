#include "GameObject.h"

using namespace DirectX;
using namespace Microsoft::WRL;

DirectX::XMFLOAT3 GameObject::GetPosition() const
{
	return XMFLOAT3(mWorldMatrix(3, 0), mWorldMatrix(3, 1), mWorldMatrix(3, 2));
}


void GameObject::SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData& meshData)
{
	// 释放旧资源
	mVertexBuffer.Reset();
	mIndexBuffer.Reset();

	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(VertexPosNormalTex);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	HR(device->CreateBuffer(&vbd, &InitData, mVertexBuffer.GetAddressOf()));


	// 设置索引缓冲区描述
	mIndexCount = (int)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(WORD) * mIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = meshData.indexVec.data();
	HR(device->CreateBuffer(&ibd, &InitData, mIndexBuffer.GetAddressOf()));



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

void GameObject::SetWorldMatrix(FXMMATRIX world)
{
	XMStoreFloat4x4(&mWorldMatrix, world);
}

void GameObject::SetTexTransformMatrix(const DirectX::XMFLOAT4X4 & texTransform)
{
	mWorldMatrix = mTexTransform;
}

void GameObject::SetTexTransformMatrix(DirectX::FXMMATRIX texTransform)
{
	XMStoreFloat4x4(&mTexTransform, texTransform);
}

void GameObject::Draw(ComPtr<ID3D11DeviceContext> deviceContext)
{

	// 设置顶点/索引缓冲区
	UINT strides = sizeof(VertexPosNormalTex);
	UINT offsets = 0;
	deviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &strides, &offsets);
	deviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// 获取之前已经绑定到渲染管线上的常量缓冲区并进行修改
	ComPtr<ID3D11Buffer> cBuffer = nullptr;
	deviceContext->VSGetConstantBuffers(0, 1, cBuffer.GetAddressOf());
	CBChangesEveryDrawing mCBDrawing;
	mCBDrawing.world = XMLoadFloat4x4(&mWorldMatrix);
	mCBDrawing.worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, mCBDrawing.world));
	mCBDrawing.texTransform = XMLoadFloat4x4(&mTexTransform);
	mCBDrawing.material = mMaterial;
	deviceContext->UpdateSubresource(cBuffer.Get(), 0, nullptr, &mCBDrawing, 0, 0);
	// 设置纹理
	deviceContext->PSSetShaderResources(0, 1, mTexture.GetAddressOf());
	// 可以开始绘制
	deviceContext->DrawIndexed(mIndexCount, 0, 0);
}
