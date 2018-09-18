#include "GameObject.h"
#include <filesystem>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std::experimental;


GameObject::GameObject()
	: mWorldMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f),
	mTexTransform(
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

const ModelPart & GameObject::GetModelPart(size_t pos) const
{
	return mParts[pos];
}

size_t GameObject::GetModelPartSize() const
{
	return mParts.size();
}

void GameObject::GetBoundingBox(BoundingBox & box) const
{
	box = mBoundingBox;
}

void GameObject::GetBoundingBox(BoundingBox & box, FXMMATRIX worldMatrix) const
{
	mBoundingBox.Transform(box, worldMatrix);
}

void GameObject::SetModel(ComPtr<ID3D11Device> device, const ObjReader & model)
{
	mParts.resize(model.objParts.size());

	// 创建包围盒
	BoundingBox::CreateFromPoints(mBoundingBox, XMLoadFloat3(&model.vMin), XMLoadFloat3(&model.vMax));

	for (size_t i = 0; i < model.objParts.size(); ++i)
	{
		auto part = model.objParts[i];

		mParts[i].vertexCount = (UINT)part.vertices.size();
		// 设置顶点缓冲区描述
		D3D11_BUFFER_DESC vbd;
		ZeroMemory(&vbd, sizeof(vbd));
		vbd.Usage = D3D11_USAGE_DEFAULT;
		vbd.ByteWidth = mParts[i].vertexCount * (UINT)sizeof(VertexPosNormalTex);
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		// 新建顶点缓冲区
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = part.vertices.data();
		HR(device->CreateBuffer(&vbd, &InitData, mParts[i].vertexBuffer.ReleaseAndGetAddressOf()));
		
		// 设置索引缓冲区描述
		D3D11_BUFFER_DESC ibd;
		ZeroMemory(&ibd, sizeof(ibd));
		ibd.Usage = D3D11_USAGE_DEFAULT;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		if (mParts[i].vertexCount > 65535)
		{
			mParts[i].indexCount = (UINT)part.indices32.size();
			mParts[i].indexFormat = DXGI_FORMAT_R32_UINT;
			ibd.ByteWidth = mParts[i].indexCount * (UINT)sizeof(DWORD);
			InitData.pSysMem = part.indices32.data();
			
		}
		else
		{
			mParts[i].indexCount = (UINT)part.indices16.size();
			mParts[i].indexFormat = DXGI_FORMAT_R16_UINT;
			ibd.ByteWidth = mParts[i].indexCount * (UINT)sizeof(WORD);
			InitData.pSysMem = part.indices16.data();
		}
		// 新建索引缓冲区
		HR(device->CreateBuffer(&ibd, &InitData, mParts[i].indexBuffer.ReleaseAndGetAddressOf()));

		// 创建环境光对应纹理
		auto& strA = part.texStrA;
		if (strA.substr(strA.size() - 3, 3) == L"dds")
		{
			HR(CreateDDSTextureFromFile(device.Get(), strA.c_str(), nullptr,
				mParts[i].texA.GetAddressOf()));
		}
		else if (part.texStrA.size() != 0)
		{
			HR(CreateWICTextureFromFile(device.Get(), strA.c_str(), nullptr,
				mParts[i].texA.GetAddressOf()));
		}
		// 创建漫射光对应纹理
		auto& strD = part.texStrD;
		if (strA == strD)
		{
			mParts[i].texD = mParts[i].texA;
		}
		else if (strD.substr(strD.size() - 3, 3) == L"dds")
		{
			HR(CreateDDSTextureFromFile(device.Get(), strD.c_str(), nullptr,
				mParts[i].texD.GetAddressOf()));
		}
		else if (part.texStrA.size() != 0)
		{
			HR(CreateWICTextureFromFile(device.Get(), strD.c_str(), nullptr,
				mParts[i].texD.GetAddressOf()));
		}

		mParts[i].material = part.material;
	}


}

void GameObject::SetMesh(ComPtr<ID3D11Device> device, const Geometry::MeshData & meshData)
{
	SetMesh(device, meshData.vertexVec, meshData.indexVec);
}

void GameObject::SetMesh(ComPtr<ID3D11Device> device, const std::vector<VertexPosNormalTex>& vertices, const std::vector<WORD> indices)
{
	SetMesh(device, vertices.data(), (UINT)vertices.size(), indices.data(),
		(UINT)indices.size(), DXGI_FORMAT_R16_UINT);	
}

void GameObject::SetMesh(ComPtr<ID3D11Device> device, const std::vector<VertexPosNormalTex>& vertices, const std::vector<DWORD> indices)
{
	SetMesh(device, vertices.data(), (UINT)vertices.size(), indices.data(),
		(UINT)indices.size(), DXGI_FORMAT_R32_UINT);
}

void GameObject::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	for (auto& part : mParts)
	{
		part.texA = part.texD = texture;
	}
}

void GameObject::SetTexture(ComPtr<ID3D11ShaderResourceView> texA, ComPtr<ID3D11ShaderResourceView> texD)
{
	for (auto& part : mParts)
	{
		part.texA = texA;
		part.texD = texD;
	}
}

void GameObject::SetTexture(size_t partIndex, ComPtr<ID3D11ShaderResourceView> texture)
{
	mParts[partIndex].texA = mParts[partIndex].texD = texture;
}

void GameObject::SetTexture(size_t partIndex, ComPtr<ID3D11ShaderResourceView> texA, ComPtr<ID3D11ShaderResourceView> texD)
{
	mParts[partIndex].texA = texA;
	mParts[partIndex].texD = texD;
}

void GameObject::SetMaterial(const Material& material)
{
	for (auto& part : mParts)
	{
		part.material = material;
	}
}

void GameObject::SetMaterial(size_t partIndex, const Material & material)
{
	mParts[partIndex].material = material;
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
	mTexTransform = texTransform;
}

void GameObject::SetTexTransformMatrix(DirectX::FXMMATRIX texTransform)
{
	XMStoreFloat4x4(&mTexTransform, texTransform);
}


void GameObject::Draw(ComPtr<ID3D11DeviceContext> deviceContext, BasicObjectFX& effect)
{

	UINT strides = sizeof(VertexPosNormalTex);
	UINT offsets = 0;

	for (auto& part : mParts)
	{
		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 1, part.vertexBuffer.GetAddressOf(), &strides, &offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);
		
		// 更新数据并应用
		effect.SetWorldMatrix(XMLoadFloat4x4(&mWorldMatrix));
		effect.SetTexTransformMatrix(XMLoadFloat4x4(&mTexTransform));
		effect.SetTextureAmbient(part.texA);
		effect.SetTextureDiffuse(part.texD);
		effect.SetMaterial(part.material);
		effect.Apply(deviceContext);
		
		deviceContext->DrawIndexed(part.indexCount, 0, 0);
	}
}

void GameObject::SetMesh(ComPtr<ID3D11Device> device, const VertexPosNormalTex * vertices, UINT vertexCount, const void * indices, UINT indexCount, DXGI_FORMAT indexFormat)
{
	mParts.resize(1);

	mParts[0].vertexCount = vertexCount;
	mParts[0].indexCount = indexCount;
	mParts[0].indexFormat = indexFormat;

	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = mParts[0].vertexCount * (UINT)sizeof(VertexPosNormalTex);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(device->CreateBuffer(&vbd, &InitData, mParts[0].vertexBuffer.ReleaseAndGetAddressOf()));

	// 设置索引缓冲区描述
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_DEFAULT;
	if (indexFormat == DXGI_FORMAT_R16_UINT)
	{
		ibd.ByteWidth = indexCount * (UINT)sizeof(WORD);
	}
	else
	{
		ibd.ByteWidth = indexCount * (UINT)sizeof(DWORD);
	}
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = indices;
	HR(device->CreateBuffer(&ibd, &InitData, mParts[0].indexBuffer.ReleaseAndGetAddressOf()));

}



