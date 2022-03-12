#include "Model.h"
#include "d3dUtil.h"
#include "DXTrace.h"
#include "tiny_obj_loader.h"
#include "TextureManager.h"

using namespace DirectX;

Model::Model()
	: modelParts(), boundingBox(), vertexStride()
{
}

Model::Model(ID3D11Device* device, const std::string& obj_path)
	: modelParts(), boundingBox(), vertexStride()
{
	SetModel(device, obj_path);
}

Model::Model(ID3D11Device* device, const void* vertices, UINT vertexSize, UINT vertexCount,
	const void* indices, UINT indexCount, DXGI_FORMAT indexFormat)
	: modelParts(), boundingBox(), vertexStride()
{
	SetMesh(device, vertices, vertexSize, vertexCount, indices, indexCount, indexFormat);
}

void Model::SetModel(ID3D11Device* device, const std::string& obj_path)
{
	if (!device)
		return;

	tinyobj::ObjReader objReader;
	objReader.ParseFromFile(obj_path);
	if (!objReader.Valid())
		return;

	std::string parent_path;
	size_t pidx = obj_path.find_last_of("/\\");
	if (pidx != std::string::npos)
		parent_path = obj_path.substr(0, pidx) + "\\";

	const auto& shapes = objReader.GetShapes();
	const auto& materials = objReader.GetMaterials();
	const auto& attrib = objReader.GetAttrib();

	modelParts.assign(shapes.size(), ModelPart{});

	vertexStride = sizeof(VertexPosNormalTex);

	// 创建总体模型的包围盒
	XMFLOAT3 vMin{ FLT_MAX, FLT_MAX, FLT_MAX }, vMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (size_t i = 0; i < attrib.vertices.size(); i += 3)
	{
		vMin.x = (std::min)(vMin.x, attrib.vertices[i]);
		vMin.y = (std::min)(vMin.y, attrib.vertices[i + 1]);
		vMin.z = (std::min)(vMin.z, attrib.vertices[i + 2]);
		vMax.x = (std::max)(vMax.x, attrib.vertices[i]);
		vMax.y = (std::max)(vMax.y, attrib.vertices[i + 1]);
		vMax.z = (std::max)(vMax.z, attrib.vertices[i + 2]);
	}
	BoundingBox::CreateFromPoints(boundingBox, XMLoadFloat3(&vMin), XMLoadFloat3(&vMax));

	
	for (size_t i = 0; i < shapes.size(); ++i)
	{
		//
		// 读取顶点和索引信息
		//
		modelParts[i].vertexCount = (uint32_t)shapes[i].mesh.num_face_vertices.size() * 3;
		modelParts[i].indexCount = (uint32_t)shapes[i].mesh.num_face_vertices.size() * 3;
		std::vector<VertexPosNormalTex> vertices(modelParts[i].vertexCount);
		std::vector<uint32_t> indices(modelParts[i].indexCount);
		size_t idx_offset = 0;
		for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); ++f)
		{
			size_t vnum = shapes[i].mesh.num_face_vertices[f];

			tinyobj::index_t idx;
			int pos_idx[3], normal_idx[3], texcoord_idx[3];
			for (size_t v = 0; v < vnum; ++v)
			{
				idx = shapes[i].mesh.indices[idx_offset + v];
				pos_idx[v] = idx.vertex_index;
				normal_idx[v] = idx.normal_index;
				texcoord_idx[v] = idx.texcoord_index;
				indices[idx_offset + v] = static_cast<uint32_t>(idx_offset + v);
			}

			for (size_t v = 0; v < vnum; ++v)
			{
				memcpy_s(&vertices[idx_offset + v].pos, sizeof(XMFLOAT3), attrib.vertices.data() + pos_idx[v] * 3, sizeof(XMFLOAT3));
				memcpy_s(&vertices[idx_offset + v].normal, sizeof(XMFLOAT3), attrib.normals.data() + normal_idx[v] * 3, sizeof(XMFLOAT3));
				memcpy_s(&vertices[idx_offset + v].tex, sizeof(XMFLOAT2), attrib.texcoords.data() + texcoord_idx[v] * 2, sizeof(XMFLOAT2));
			}

			idx_offset += vnum;
		}

		// 创建子包围盒
		vMin = XMFLOAT3{ FLT_MAX, FLT_MAX, FLT_MAX }, vMax = XMFLOAT3{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			vMin.x = (std::min)(vMin.x, vertices[i].pos.x);
			vMin.y = (std::min)(vMin.y, vertices[i].pos.y);
			vMin.z = (std::min)(vMin.z, vertices[i].pos.z);
			vMax.x = (std::max)(vMax.x, vertices[i].pos.x);
			vMax.y = (std::max)(vMax.y, vertices[i].pos.y);
			vMax.z = (std::max)(vMax.z, vertices[i].pos.z);
		}
		BoundingBox::CreateFromPoints(modelParts[i].boundingBox, XMLoadFloat3(&vMin), XMLoadFloat3(&vMax));


		// 设置顶点缓冲区描述
		D3D11_BUFFER_DESC vbd;
		ZeroMemory(&vbd, sizeof(vbd));
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = modelParts[i].vertexCount * (UINT)sizeof(VertexPosNormalTex);
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		// 新建顶点缓冲区
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = vertices.data();
		HR(device->CreateBuffer(&vbd, &InitData, modelParts[i].vertexBuffer.ReleaseAndGetAddressOf()));

		// 设置索引缓冲区描述
		D3D11_BUFFER_DESC ibd;
		ZeroMemory(&ibd, sizeof(ibd));
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		modelParts[i].indexCount = modelParts[i].indexCount;
		modelParts[i].indexFormat = DXGI_FORMAT_R32_UINT;
		ibd.ByteWidth = modelParts[i].indexCount * (uint32_t)sizeof(uint32_t);
		InitData.pSysMem = indices.data();
		// 新建索引缓冲区
		HR(device->CreateBuffer(&ibd, &InitData, modelParts[i].indexBuffer.ReleaseAndGetAddressOf()));

		//
		// 读取材质纹理
		//
		memcpy_s(&modelParts[i].ambient, sizeof(float) * 3, 
			materials[shapes[i].mesh.material_ids[0]].ambient, sizeof(float) * 3);
		memcpy_s(&modelParts[i].diffuse, sizeof(float) * 3,
			materials[shapes[i].mesh.material_ids[0]].diffuse, sizeof(float) * 3);
		memcpy_s(&modelParts[i].specular, sizeof(float) * 3,
			materials[shapes[i].mesh.material_ids[0]].specular, sizeof(float) * 3);
		modelParts[i].ambient.w = modelParts[i].diffuse.w = modelParts[i].specular.w = 1.0f;
		
		// 漫反射贴图
		auto diffuse_texpath = UTF8ToWString(parent_path + materials[shapes[i].mesh.material_ids[0]].diffuse_texname);
		modelParts[i].texDiffuse = TextureManager::Get().CreateTexture(diffuse_texpath.c_str(), true, true);

		// 法线贴图
		auto normal_texpath = UTF8ToWString(parent_path + materials[shapes[i].mesh.material_ids[0]].diffuse_texname);
		modelParts[i].texNormalMap = TextureManager::Get().CreateTexture(normal_texpath.c_str());
		
	}

}

void Model::SetMesh(ID3D11Device* device, const void* vertices, UINT vertexSize, UINT vertexCount, const void* indices, UINT indexCount, DXGI_FORMAT indexFormat)
{
	vertexStride = vertexSize;

	modelParts.resize(1);

	modelParts[0].vertexCount = vertexCount;
	modelParts[0].indexCount = indexCount;
	modelParts[0].indexFormat = indexFormat;

	modelParts[0].ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	modelParts[0].diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	modelParts[0].specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = vertexSize * vertexCount;
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
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
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

void XM_CALLCONV Model::FrustumCulling(const DirectX::BoundingFrustum& frustumInWorld, DirectX::XMMATRIX World)
{
	BoundingBox bbox;
	this->boundingBox.Transform(bbox, World);
	if (bbox.Intersects(frustumInWorld))
	{
		for (auto& modelPart : modelParts)
		{
			modelPart.boundingBox.Transform(bbox, World);
			modelPart.inFrustum = bbox.Intersects(frustumInWorld);
		}
	}
	else
	{
		for (auto& modelPart : modelParts)
			modelPart.inFrustum = false;
	}
}


void Model::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)

	size_t modelPartSize = modelParts.size();
	for (size_t i = 0; i < modelPartSize; ++i)
	{
		D3D11SetDebugObjectName(modelParts[i].vertexBuffer.Get(), name + ".part[" + std::to_string(i) + "].VertexBuffer");
		D3D11SetDebugObjectName(modelParts[i].indexBuffer.Get(), name + ".part[" + std::to_string(i) + "].IndexBuffer");
	}

#else
	UNREFERENCED_PARAMETER(name);
#endif
}




