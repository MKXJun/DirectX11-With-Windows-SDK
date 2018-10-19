#include "Model.h"

using namespace DirectX;

Model::Model()
{
}

Model::Model(ComPtr<ID3D11Device> device, const ObjReader & model)
{
	SetModel(device, model);
}

Model::Model(ComPtr<ID3D11Device> device, const Geometry::MeshData & meshData)
{
	SetMesh(device, meshData);
}

Model::Model(ComPtr<ID3D11Device> device, const std::vector<VertexPosNormalTex>& vertices, const std::vector<WORD>& indices)
{
	SetMesh(device, vertices, indices);
}

Model::Model(ComPtr<ID3D11Device> device, const std::vector<VertexPosNormalTex>& vertices, const std::vector<DWORD>& indices)
{
	SetMesh(device, vertices, indices);
}

Model::Model(ComPtr<ID3D11Device> device, const VertexPosNormalTex * vertices, UINT vertexCount, const void * indices, UINT indexCount, DXGI_FORMAT indexFormat)
{
	SetMesh(device, vertices, vertexCount, indices, indexCount, indexFormat);
}

void Model::SetModel(ComPtr<ID3D11Device> device, const ObjReader & model)
{
	modelParts.resize(model.objParts.size());

	// 创建包围盒
	BoundingBox::CreateFromPoints(boundingBox, XMLoadFloat3(&model.vMin), XMLoadFloat3(&model.vMax));

	for (size_t i = 0; i < model.objParts.size(); ++i)
	{
		auto part = model.objParts[i];

		modelParts[i].vertexCount = (UINT)part.vertices.size();
		// 设置顶点缓冲区描述
		D3D11_BUFFER_DESC vbd;
		ZeroMemory(&vbd, sizeof(vbd));
		vbd.Usage = D3D11_USAGE_DEFAULT;
		vbd.ByteWidth = modelParts[i].vertexCount * (UINT)sizeof(VertexPosNormalTex);
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		// 新建顶点缓冲区
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = part.vertices.data();
		HR(device->CreateBuffer(&vbd, &InitData, modelParts[i].vertexBuffer.ReleaseAndGetAddressOf()));

		// 设置索引缓冲区描述
		D3D11_BUFFER_DESC ibd;
		ZeroMemory(&ibd, sizeof(ibd));
		ibd.Usage = D3D11_USAGE_DEFAULT;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		if (modelParts[i].vertexCount > 65535)
		{
			modelParts[i].indexCount = (UINT)part.indices32.size();
			modelParts[i].indexFormat = DXGI_FORMAT_R32_UINT;
			ibd.ByteWidth = modelParts[i].indexCount * (UINT)sizeof(DWORD);
			InitData.pSysMem = part.indices32.data();

		}
		else
		{
			modelParts[i].indexCount = (UINT)part.indices16.size();
			modelParts[i].indexFormat = DXGI_FORMAT_R16_UINT;
			ibd.ByteWidth = modelParts[i].indexCount * (UINT)sizeof(WORD);
			InitData.pSysMem = part.indices16.data();
		}
		// 新建索引缓冲区
		HR(device->CreateBuffer(&ibd, &InitData, modelParts[i].indexBuffer.ReleaseAndGetAddressOf()));

		// 创建环境光对应纹理
		auto& strA = part.texStrA;
		if (strA.substr(strA.size() - 3, 3) == L"dds")
		{
			HR(CreateDDSTextureFromFile(device.Get(), strA.c_str(), nullptr,
				modelParts[i].texA.GetAddressOf()));
		}
		else if (part.texStrA.size() != 0)
		{
			HR(CreateWICTextureFromFile(device.Get(), strA.c_str(), nullptr,
				modelParts[i].texA.GetAddressOf()));
		}
		// 创建漫射光对应纹理
		auto& strD = part.texStrD;
		if (strA == strD)
		{
			modelParts[i].texD = modelParts[i].texA;
		}
		else if (strD.substr(strD.size() - 3, 3) == L"dds")
		{
			HR(CreateDDSTextureFromFile(device.Get(), strD.c_str(), nullptr,
				modelParts[i].texD.GetAddressOf()));
		}
		else if (part.texStrA.size() != 0)
		{
			HR(CreateWICTextureFromFile(device.Get(), strD.c_str(), nullptr,
				modelParts[i].texD.GetAddressOf()));
		}

		modelParts[i].material = part.material;
	}


}

void Model::SetMesh(ComPtr<ID3D11Device> device, const Geometry::MeshData & meshData)
{
	SetMesh(device, meshData.vertexVec, meshData.indexVec);
}

void Model::SetMesh(ComPtr<ID3D11Device> device, const std::vector<VertexPosNormalTex>& vertices, const std::vector<WORD>& indices)
{
	SetMesh(device, vertices.data(), (UINT)vertices.size(), indices.data(),
		(UINT)indices.size(), DXGI_FORMAT_R16_UINT);
}

void Model::SetMesh(ComPtr<ID3D11Device> device, const std::vector<VertexPosNormalTex>& vertices, const std::vector<DWORD>& indices)
{
	SetMesh(device, vertices.data(), (UINT)vertices.size(), indices.data(),
		(UINT)indices.size(), DXGI_FORMAT_R32_UINT);
}

void Model::SetMesh(ComPtr<ID3D11Device> device, const VertexPosNormalTex * vertices, UINT vertexCount, const void * indices, UINT indexCount, DXGI_FORMAT indexFormat)
{
	modelParts.resize(1);

	modelParts[0].vertexCount = vertexCount;
	modelParts[0].indexCount = indexCount;
	modelParts[0].indexFormat = indexFormat;

	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = modelParts[0].vertexCount * (UINT)sizeof(VertexPosNormalTex);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(device->CreateBuffer(&vbd, &InitData, modelParts[0].vertexBuffer.ReleaseAndGetAddressOf()));

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
	HR(device->CreateBuffer(&ibd, &InitData, modelParts[0].indexBuffer.ReleaseAndGetAddressOf()));

}



