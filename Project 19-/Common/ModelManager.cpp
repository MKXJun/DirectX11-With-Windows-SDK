#include "XUtil.h"
#include "ModelManager.h"
#include "TextureManager.h"

#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace DirectX;

void Model::CreateFromFile(Model& model, ID3D11Device* device, std::string_view filename)
{
    using namespace Assimp;
    namespace fs = std::filesystem;
    
    model.materials.clear();
    model.meshdatas.clear();
    model.boundingbox = BoundingBox();

    Importer importer;

    auto pAssimpScene = importer.ReadFile(filename.data(), aiProcess_ConvertToLeftHanded |
        aiProcess_GenBoundingBoxes | aiProcess_Triangulate | aiProcess_ImproveCacheLocality);

    if (pAssimpScene && !(pAssimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && pAssimpScene->HasMeshes())
    {
        model.meshdatas.resize(pAssimpScene->mNumMeshes);
        model.materials.resize(pAssimpScene->mNumMaterials);
        for (uint32_t i = 0; i < pAssimpScene->mNumMeshes; ++i)
        {
            auto& mesh = model.meshdatas[i];

            auto pAiMesh = pAssimpScene->mMeshes[i];
            uint32_t numVertices = pAiMesh->mNumVertices;

            CD3D11_BUFFER_DESC bufferDesc(0, D3D11_BIND_VERTEX_BUFFER);
            D3D11_SUBRESOURCE_DATA initData{ nullptr, 0, 0 };
            // 位置
            if (pAiMesh->mNumVertices > 0)
            {
                initData.pSysMem = pAiMesh->mVertices;
                bufferDesc.ByteWidth = numVertices * sizeof(XMFLOAT3);
                device->CreateBuffer(&bufferDesc, &initData, mesh.m_pVertices.GetAddressOf());

                BoundingBox::CreateFromPoints(mesh.m_BoundingBox, numVertices,
                    (const XMFLOAT3*)pAiMesh->mVertices, sizeof(XMFLOAT3));
                if (i == 0)
                    model.boundingbox = mesh.m_BoundingBox;
                else
                    model.boundingbox.CreateMerged(model.boundingbox, model.boundingbox, mesh.m_BoundingBox);
            }

            // 法线
            if (pAiMesh->HasNormals())
            {
                initData.pSysMem = pAiMesh->mNormals;
                bufferDesc.ByteWidth = numVertices * sizeof(XMFLOAT3);
                device->CreateBuffer(&bufferDesc, &initData, mesh.m_pNormals.GetAddressOf());
            }

            // 切线和副切线
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
                device->CreateBuffer(&bufferDesc, &initData, mesh.m_pTangents.GetAddressOf());

                for (uint32_t i = 0; i < pAiMesh->mNumVertices; ++i)
                {
                    memcpy_s(&tangents[i], sizeof(XMFLOAT3),
                        pAiMesh->mBitangents + i, sizeof(XMFLOAT3));
                }
                device->CreateBuffer(&bufferDesc, &initData, mesh.m_pBitangents.GetAddressOf());
            }

            // 纹理坐标
            uint32_t numUVs = 8;
            while (numUVs && !pAiMesh->HasTextureCoords(numUVs - 1))
                numUVs--;

            if (numUVs > 0)
            {
                mesh.m_pTexcoordArrays.resize(numUVs);
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
                    device->CreateBuffer(&bufferDesc, &initData, mesh.m_pTexcoordArrays[i].GetAddressOf());
                }
            }

            // 索引
            uint32_t numFaces = pAiMesh->mNumFaces;
            uint32_t numIndices = numFaces * 3;
            if (numFaces > 0)
            {
                mesh.m_IndexCount = numIndices;
                if (numIndices < 65535)
                {
                    std::vector<uint16_t> indices(numIndices);
                    for (size_t i = 0; i < numFaces; ++i)
                    {
                        indices[i * 3] = static_cast<uint16_t>(pAiMesh->mFaces[i].mIndices[0]);
                        indices[i * 3 + 1] = static_cast<uint16_t>(pAiMesh->mFaces[i].mIndices[1]);
                        indices[i * 3 + 2] = static_cast<uint16_t>(pAiMesh->mFaces[i].mIndices[2]);
                    }
                    bufferDesc = CD3D11_BUFFER_DESC(numIndices * sizeof(uint16_t), D3D11_BIND_INDEX_BUFFER);
                    initData.pSysMem = indices.data();
                    device->CreateBuffer(&bufferDesc, &initData, mesh.m_pIndices.GetAddressOf());
                }
                else
                {
                    std::vector<uint32_t> indices(numIndices);
                    for (size_t i = 0; i < numFaces; ++i)
                    {
                        memcpy_s(indices.data() + i * 3, sizeof(uint32_t) * 3,
                            pAiMesh->mFaces[i].mIndices, sizeof(uint32_t) * 3);
                    }
                    bufferDesc = CD3D11_BUFFER_DESC(numIndices * sizeof(uint32_t), D3D11_BIND_INDEX_BUFFER);
                    initData.pSysMem = indices.data();
                    device->CreateBuffer(&bufferDesc, &initData, mesh.m_pIndices.GetAddressOf());
                }
            }

            // 材质索引
            mesh.m_MaterialIndex = pAiMesh->mMaterialIndex;
        }


        for (uint32_t i = 0; i < pAssimpScene->mNumMaterials; ++i)
        {
            auto& material = model.materials[i];

            auto pAiMaterial = pAssimpScene->mMaterials[i];
            XMFLOAT4 vec{};
            float value{};
            uint32_t boolean{};
            uint32_t num = 3;

            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, (float*)&vec, &num))
                material.Set("$AmbientColor", vec);
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, (float*)&vec, &num))
                material.Set("$DiffuseColor", vec);
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, (float*)&vec, &num))
                material.Set("$SpecularColor", vec);
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_SPECULAR_FACTOR, value))
                material.Set("$SpecularFactor", value);
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, (float*)&vec, &num))
                material.Set("$EmissiveColor", vec);
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_OPACITY, value))
                material.Set("$Opacity", value);
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, (float*)&vec, &num))
                material.Set("$TransparentColor", vec);
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_REFLECTIVE, (float*)&vec, &num))
                material.Set("$ReflectiveColor", vec);
            if (pAiMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                aiString aiPath;
                pAiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiPath);
                fs::path tex_filename = filename;
                tex_filename = tex_filename.parent_path() / aiPath.C_Str();
                TextureManager::Get().CreateTexture(tex_filename.string(), true, true);
                material.Set("$Diffuse", tex_filename.string());
            }
            if (pAiMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
            {
                aiString aiPath;
                pAiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiPath);
                fs::path tex_filename = filename;
                tex_filename = tex_filename.parent_path() / aiPath.C_Str();
                TextureManager::Get().CreateTexture(tex_filename.string());
                material.Set("$Normal", tex_filename.string());
            }
        }
    }
}

void Model::CreateFromGeometry(Model& model, ID3D11Device* device, const GeometryData& data, bool isDynamic)
{
    // 默认材质
    model.materials = { Material{} };
    model.materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
    model.materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
    model.materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
    model.materials[0].Set<float>("$SpecularFactor", 10.0f);
    model.materials[0].Set<float>("$Opacity", 1.0f);

    model.meshdatas = { MeshData{} };
    model.meshdatas[0].m_pTexcoordArrays.resize(1);
    model.meshdatas[0].m_VertexCount = (uint32_t)data.vertices.size();
    model.meshdatas[0].m_IndexCount = (uint32_t)(!data.indices16.empty() ? data.indices16.size() : data.indices32.size());
    model.meshdatas[0].m_MaterialIndex = 0;

    CD3D11_BUFFER_DESC bufferDesc(0,
        D3D11_BIND_VERTEX_BUFFER,
        isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
        isDynamic ? D3D11_CPU_ACCESS_WRITE : 0);
    D3D11_SUBRESOURCE_DATA initData{ nullptr, 0, 0 };

    initData.pSysMem = data.vertices.data();
    bufferDesc.ByteWidth = (uint32_t)data.vertices.size() * sizeof(XMFLOAT3);
    device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pVertices.GetAddressOf());

    if (!data.normals.empty())
    {
        initData.pSysMem = data.normals.data();
        bufferDesc.ByteWidth = (uint32_t)data.normals.size() * sizeof(XMFLOAT3);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pNormals.GetAddressOf());
    }

    if (!data.texcoords.empty())
    {
        initData.pSysMem = data.texcoords.data();
        bufferDesc.ByteWidth = (uint32_t)data.texcoords.size() * sizeof(XMFLOAT2);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pTexcoordArrays[0].GetAddressOf());
    }

    if (!data.tangents.empty())
    {
        initData.pSysMem = data.tangents.data();
        bufferDesc.ByteWidth = (uint32_t)data.tangents.size() * sizeof(XMFLOAT4);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pTangents.GetAddressOf());
    }

    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.CPUAccessFlags = 0;
    if (!data.indices16.empty())
    {
        initData.pSysMem = data.indices16.data();
        bufferDesc = CD3D11_BUFFER_DESC((uint16_t)data.indices16.size() * sizeof(uint16_t), D3D11_BIND_INDEX_BUFFER);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pIndices.GetAddressOf());
    }
    else
    {
        initData.pSysMem = data.indices32.data();
        bufferDesc = CD3D11_BUFFER_DESC((uint32_t)data.indices32.size() * sizeof(uint32_t), D3D11_BIND_INDEX_BUFFER);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pIndices.GetAddressOf());
    }
}

void Model::SetDebugObjectName(std::string_view name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    std::string baseStr = name.data();
    size_t sz = meshdatas.size();
    std::string str;
    str.reserve(100);
    for (size_t i = 0; i < sz; ++i)
    {
        baseStr = name.data();
        baseStr += "[" + std::to_string(i) + "].";
        if (meshdatas[i].m_pVertices)
            ::SetDebugObjectName(meshdatas[i].m_pVertices.Get(), baseStr + "vertices");
        if (meshdatas[i].m_pNormals)
            ::SetDebugObjectName(meshdatas[i].m_pNormals.Get(), baseStr + "normals");
        if (meshdatas[i].m_pTangents)
            ::SetDebugObjectName(meshdatas[i].m_pTangents.Get(), baseStr + "tangents");
        if (meshdatas[i].m_pBitangents)
            ::SetDebugObjectName(meshdatas[i].m_pBitangents.Get(), baseStr + "bitangents");
        if (meshdatas[i].m_pColors)
            ::SetDebugObjectName(meshdatas[i].m_pColors.Get(), baseStr + "colors");
        if (!meshdatas[i].m_pTexcoordArrays.empty())
        {
            size_t texSz = meshdatas[i].m_pTexcoordArrays.size();
            for (size_t j = 0; j < texSz; ++j)
                ::SetDebugObjectName(meshdatas[i].m_pTexcoordArrays[j].Get(), baseStr + "uv" + std::to_string(j));
        }
        if (meshdatas[i].m_pIndices)
            ::SetDebugObjectName(meshdatas[i].m_pIndices.Get(), baseStr + "indices");
    }
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

namespace
{
    // ModelManager单例
    ModelManager* s_pInstance = nullptr;
}


ModelManager::ModelManager()
{
    if (s_pInstance)
        throw std::exception("ModelManager is a singleton!");
    s_pInstance = this;
}

ModelManager::~ModelManager()
{
}

ModelManager& ModelManager::Get()
{
    if (!s_pInstance)
        throw std::exception("ModelManager needs an instance!");
    return *s_pInstance;
}

void ModelManager::Init(ID3D11Device* device)
{
    m_pDevice = device;
    m_pDevice->GetImmediateContext(m_pDeviceContext.ReleaseAndGetAddressOf());
}

Model* ModelManager::CreateFromFile(std::string_view filename)
{
    return CreateFromFile(filename, filename);
}

Model* ModelManager::CreateFromFile(std::string_view name, std::string_view filename)
{
    XID modelID = StringToID(name);
    auto& model = m_Models[modelID];
    Model::CreateFromFile(model, m_pDevice.Get(), filename);
    return &model;
}

Model* ModelManager::CreateFromGeometry(std::string_view name, const GeometryData& data, bool isDynamic)
{
    XID modelID = StringToID(name);
    auto& model = m_Models[modelID];
    Model::CreateFromGeometry(model, m_pDevice.Get(), data, isDynamic);

    return &model;
}

const Model* ModelManager::GetModel(std::string_view name) const
{
    XID nameID = StringToID(name);
    if (auto it = m_Models.find(nameID); it != m_Models.end())
        return &it->second;
    return nullptr;
}

Model* ModelManager::GetModel(std::string_view name)
{
    XID nameID = StringToID(name);
    if (m_Models.count(nameID))
        return &m_Models[nameID];
    return nullptr;
}
