#include "XUtil.h"
#include "GameObject.h"
#include "DXTrace.h"
#include "TextureManager.h"

#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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

size_t GameObject::GetMeshCount() const
{
	return m_pMeshDatas.size();
}

size_t GameObject::GetMaterialCount() const
{
	return m_pMaterials.size();
}

const Material* GameObject::GetMaterial(size_t idx) const
{
	return (idx < m_pMaterials.size() ? m_pMaterials[idx].get() : nullptr);
}

Material* GameObject::GetMaterial(size_t idx)
{
	return (idx < m_pMaterials.size() ? m_pMaterials[idx].get() : nullptr);
}

BoundingBox GameObject::GetBoundingBox() const
{
	BoundingBox box;
	m_BoundingBox.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}

BoundingBox GameObject::GetBoundingBox(size_t idx) const
{
	BoundingBox box;
	m_pMeshDatas[idx]->m_BoundingBox.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}

BoundingBox GameObject::GetLocalBoundingBox() const
{
	return m_BoundingBox;
}

BoundingBox GameObject::GetLocalBoundingBox(size_t idx) const
{
	return m_pMeshDatas[idx]->m_BoundingBox;
}


BoundingOrientedBox GameObject::GetBoundingOrientedBox() const
{
	BoundingOrientedBox box;
	BoundingOrientedBox::CreateFromBoundingBox(box, m_BoundingBox);
	box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}

BoundingOrientedBox GameObject::GetBoundingOrientedBox(size_t idx) const
{
	BoundingOrientedBox box;
	BoundingOrientedBox::CreateFromBoundingBox(box, m_pMeshDatas[idx]->m_BoundingBox);
	box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}

void GameObject::FrustumCulling(const BoundingFrustum& frustumInWorld)
{
	size_t sz = m_pMeshDatas.size();
	m_InFrustum = false;
	for (size_t i = 0; i < sz; ++i)
	{
		m_pMeshDatas[i]->m_InFrustum = frustumInWorld.Intersects(GetBoundingOrientedBox(i));
		m_InFrustum = m_InFrustum || m_pMeshDatas[i]->m_InFrustum;
	}
}

void GameObject::CubeCulling(const DirectX::BoundingOrientedBox& obbInWorld)
{
	size_t sz = m_pMeshDatas.size();
	m_InFrustum = false;
	for (size_t i = 0; i < sz; ++i)
	{
		m_pMeshDatas[i]->m_InFrustum = obbInWorld.Intersects(GetBoundingOrientedBox(i));
		m_InFrustum = m_InFrustum || m_pMeshDatas[i]->m_InFrustum;
	}
}

void GameObject::LoadModelFromFile(ID3D11Device* device, std::string_view filename)
{
	using namespace Assimp;
	namespace fs = std::filesystem;
	Importer importer;

	auto pAssimpScene = importer.ReadFile(filename.data(), aiProcess_ConvertToLeftHanded |
		aiProcess_GenBoundingBoxes | aiProcess_Triangulate | aiProcess_ImproveCacheLocality);

	if (pAssimpScene && !(pAssimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && pAssimpScene->HasMeshes())
	{
		m_pMeshDatas.resize(pAssimpScene->mNumMeshes);
		m_pMaterials.resize(pAssimpScene->mNumMaterials);
		for (uint32_t i = 0; i < pAssimpScene->mNumMeshes; ++i)
		{
			auto& pMesh = m_pMeshDatas[i];
			pMesh = std::make_shared<MeshData>();

			auto pAiMesh = pAssimpScene->mMeshes[i];
			uint32_t numVertices = pAiMesh->mNumVertices;

			CD3D11_BUFFER_DESC bufferDesc(0, D3D11_BIND_VERTEX_BUFFER);
			D3D11_SUBRESOURCE_DATA initData{ nullptr, 0, 0 };
			// 位置
			if (pAiMesh->mNumVertices > 0)
			{
				initData.pSysMem = pAiMesh->mVertices;
				bufferDesc.ByteWidth = numVertices * sizeof(XMFLOAT3);
				device->CreateBuffer(&bufferDesc, &initData, pMesh->m_pVertices.GetAddressOf());
				
				BoundingBox::CreateFromPoints(pMesh->m_BoundingBox, numVertices,
					(const XMFLOAT3*)pAiMesh->mVertices, sizeof(XMFLOAT3));
				XMVECTOR vMin = XMLoadFloat3(&pMesh->m_BoundingBox.Center) + XMLoadFloat3(&pMesh->m_BoundingBox.Extents);
				if (i == 0)
					m_BoundingBox = pMesh->m_BoundingBox;
				else
					m_BoundingBox.CreateMerged(m_BoundingBox, m_BoundingBox, pMesh->m_BoundingBox);
			}

			// 法线
			if (pAiMesh->HasNormals())
			{
				initData.pSysMem = pAiMesh->mNormals;
				bufferDesc.ByteWidth = numVertices * sizeof(XMFLOAT3);
				device->CreateBuffer(&bufferDesc, &initData, pMesh->m_pNormals.GetAddressOf());
			}

			// 切线
			if (pAiMesh->HasTangentsAndBitangents())
			{
				std::vector<XMFLOAT4> tangents(numVertices, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
				for (uint32_t i = 0; i < pAiMesh->mNumVertices; ++i)
				{
					memcpy_s(&tangents[i], sizeof(XMFLOAT3), 
						pAiMesh->mTangents + i, sizeof(XMFLOAT3));
				}

				initData.pSysMem = tangents.data();
				bufferDesc.ByteWidth = pAiMesh->mNumVertices * sizeof(XMFLOAT4);
				device->CreateBuffer(&bufferDesc, &initData, pMesh->m_pTangents.GetAddressOf());
			}

			// 纹理坐标
			uint32_t numUVs = 8;
			while (numUVs && !pAiMesh->HasTextureCoords(numUVs - 1))
				numUVs--;

			if (numUVs > 0)
			{
				pMesh->m_pTexcoordArrays.resize(numUVs);
				for (uint32_t i = 0; i < numUVs; ++i)
				{
					std::vector<XMFLOAT2> uvs(numVertices);
					for (uint32_t j = 0; j < numVertices; ++j)
					{
						memcpy_s(&uvs[j], sizeof(XMFLOAT2),
							pAiMesh->mTextureCoords[i] + j, sizeof(XMFLOAT2));
					}
					initData.pSysMem = uvs.data();
					bufferDesc.ByteWidth = numVertices * sizeof(XMFLOAT2);
					device->CreateBuffer(&bufferDesc, &initData, pMesh->m_pTexcoordArrays[i].GetAddressOf());
				}
			}

			// 索引
			uint32_t numFaces = pAiMesh->mNumFaces;
			uint32_t numIndices = numFaces * 3;
			if (numFaces > 0)
			{
				std::vector<uint32_t> indices(numIndices);
				uint32_t offset;
				for (uint32_t i = 0; i < numFaces; ++i)
				{
					offset = i * 3;
					memcpy_s(indices.data() + offset, sizeof(uint32_t) * 3,
						pAiMesh->mFaces[i].mIndices, sizeof(uint32_t) * 3);
				}
				bufferDesc = CD3D11_BUFFER_DESC(numIndices * sizeof(uint32_t), D3D11_BIND_INDEX_BUFFER);
				initData.pSysMem = indices.data();
				device->CreateBuffer(&bufferDesc, &initData, pMesh->m_pIndices.GetAddressOf());
				pMesh->m_IndexCount = numIndices;
			}

			// 材质索引
			pMesh->m_MaterialIndex = pAiMesh->mMaterialIndex;
		}


		for (uint32_t i = 0; i < pAssimpScene->mNumMaterials; ++i)
		{
			auto& pMaterial = m_pMaterials[i];
			pMaterial = std::make_shared<Material>();

			auto pAiMaterial = pAssimpScene->mMaterials[i];
			XMFLOAT4 vec{};
			float value{};
			uint32_t boolean{};
			uint32_t num = 3;
			
			if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, (float*)&vec, &num))
				pMaterial->Set("$AmbientColor", vec);
			if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, (float*)&vec, &num))
				pMaterial->Set("$DiffuseColor", vec);
			if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, (float*)&vec, &num))
				pMaterial->Set("$SpecularColor", vec);
			if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, (float*)&vec, &num))
				pMaterial->Set("$EmissiveColor", vec);
			if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, (float*)&vec, &num))
				pMaterial->Set("$TransparentColor", vec);
			if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_REFLECTIVE, (float*)&vec, &num))
				pMaterial->Set("$ReflectiveColor", vec);
			if (pAiMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				aiString aiPath;
				pAiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiPath);
				fs::path tex_filename = filename;
				tex_filename = tex_filename.parent_path() / aiPath.C_Str();
				TextureManager::Get().CreateTexture(tex_filename.string(), true, true);
				pMaterial->SetTexture("$Diffuse", tex_filename.string());
			}
			if (pAiMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
			{
				aiString aiPath;
				pAiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiPath);
				fs::path tex_filename = filename;
				tex_filename = tex_filename.parent_path() / aiPath.C_Str();
				TextureManager::Get().CreateTexture(tex_filename.string());
				pMaterial->SetTexture("$Normal", tex_filename.string());
			}
		}
	}
}

void GameObject::LoadModelFromGeometry(ID3D11Device* device, const Geometry::MeshData& meshData)
{
	m_pMaterials.resize(1, std::make_shared<Material>());
	m_pMeshDatas.resize(1, std::make_shared<MeshData>());
	m_pMeshDatas[0]->m_pTexcoordArrays.resize(1);
	m_pMeshDatas[0]->m_IndexCount = (uint32_t)meshData.indices.size();
	m_pMeshDatas[0]->m_MaterialIndex = 0;
	CD3D11_BUFFER_DESC bufferDesc(0, D3D11_BIND_VERTEX_BUFFER);
	D3D11_SUBRESOURCE_DATA initData{ nullptr, 0, 0 };

	initData.pSysMem = meshData.vertices.data();
	bufferDesc.ByteWidth = (uint32_t)meshData.vertices.size() * sizeof(XMFLOAT3);
	device->CreateBuffer(&bufferDesc, &initData, m_pMeshDatas[0]->m_pVertices.ReleaseAndGetAddressOf());

	initData.pSysMem = meshData.normals.data();
	bufferDesc.ByteWidth = (uint32_t)meshData.normals.size() * sizeof(XMFLOAT3);
	device->CreateBuffer(&bufferDesc, &initData, m_pMeshDatas[0]->m_pNormals.ReleaseAndGetAddressOf());

	initData.pSysMem = meshData.texcoords.data();
	bufferDesc.ByteWidth = (uint32_t)meshData.texcoords.size() * sizeof(XMFLOAT2);
	device->CreateBuffer(&bufferDesc, &initData, m_pMeshDatas[0]->m_pTexcoordArrays[0].ReleaseAndGetAddressOf());

	initData.pSysMem = meshData.tangents.data();
	bufferDesc.ByteWidth = (uint32_t)meshData.tangents.size() * sizeof(XMFLOAT4);
	device->CreateBuffer(&bufferDesc, &initData, m_pMeshDatas[0]->m_pTangents.ReleaseAndGetAddressOf());

	initData.pSysMem = meshData.indices.data();
	bufferDesc = CD3D11_BUFFER_DESC((uint32_t)meshData.indices.size() * sizeof(uint32_t), D3D11_BIND_INDEX_BUFFER);
	device->CreateBuffer(&bufferDesc, &initData, m_pMeshDatas[0]->m_pIndices.ReleaseAndGetAddressOf());
}



void GameObject::Draw(ID3D11DeviceContext * deviceContext, IEffect * effect)
{

	for (auto& pMeshData : m_pMeshDatas)
	{
		if (!pMeshData->m_InFrustum)
			continue;

		IEffectMeshData* pEffectMeshData = dynamic_cast<IEffectMeshData*>(effect);
		if (!pEffectMeshData)
			continue;

		IEffectMaterial* pEffectMaterial = dynamic_cast<IEffectMaterial*>(effect);
		if (pEffectMaterial)
			pEffectMaterial->SetMaterial(*m_pMaterials[pMeshData->m_MaterialIndex]);

		IEffectTransform* pEffectTransform = dynamic_cast<IEffectTransform*>(effect);
		if (pEffectTransform)
			pEffectTransform->SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());

		effect->Apply(deviceContext);

		MeshDataInput input = pEffectMeshData->GetInputData(*pMeshData);
		{
			deviceContext->IASetVertexBuffers(0, (uint32_t)input.pVertexBuffers.size(), 
				input.pVertexBuffers.data(), input.strides.data(), input.offsets.data());
			deviceContext->IASetIndexBuffer(input.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

			deviceContext->DrawIndexed(input.indexCount, 0, 0);
		}
		
	}
}

void GameObject::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)

	/*m_Model.SetDebugObjectName(name);
	if (m_pInstancedBuffer)
	{
		D3D11SetDebugObjectName(m_pInstancedBuffer.Get(), name + ".InstancedBuffer");
	}*/

#endif
}


