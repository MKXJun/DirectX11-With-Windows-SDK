//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易游戏对象
// Simple game object.
//***************************************************************************************

#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Effects.h"
#include <Geometry.h>
#include <Transform.h>

// 一个尽可能小的游戏对象类
class GameObject
{
public:
    // 使用模板别名(C++11)简化类型名
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    GameObject();

    // 获取物体变换
    Transform& GetTransform();
    // 获取物体变换
    const Transform& GetTransform() const;

    // 设置缓冲区
    template<class VertexType, class IndexType>
    void SetBuffer(ID3D11Device* device, const Geometry::MeshData<VertexType, IndexType>& meshData);
    // 设置纹理
    void SetTexture(ID3D11ShaderResourceView* texture);
    // 设置材质
    void SetMaterial(const Material& material);

    // 绘制
    void Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect);

    // 设置调试对象名
    // 若缓冲区被重新设置，调试对象名也需要被重新设置
    void SetDebugObjectName(const std::string& name);
private:
    Transform m_Transform;								// 物体变换信息
    Material m_Material;								// 物体材质
    ComPtr<ID3D11ShaderResourceView> m_pTexture;		// 纹理
    ComPtr<ID3D11Buffer> m_pVertexBuffer;				// 顶点缓冲区
    ComPtr<ID3D11Buffer> m_pIndexBuffer;				// 索引缓冲区
    UINT m_VertexStride;								// 顶点字节大小
    UINT m_IndexCount;								    // 索引数目	
};

template<class VertexType, class IndexType>
inline void GameObject::SetBuffer(ID3D11Device * device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{
    // 释放旧资源
    m_pVertexBuffer.Reset();
    m_pIndexBuffer.Reset();

    // 检查D3D设备
    if (device == nullptr)
        return;

    // 设置顶点缓冲区描述
    m_VertexStride = sizeof(VertexType);
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = (UINT)meshData.vertexVec.size() * m_VertexStride;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = meshData.vertexVec.data();
    device->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf());

    // 设置索引缓冲区描述
    m_IndexCount = (UINT)meshData.indexVec.size();
    D3D11_BUFFER_DESC ibd;
    ZeroMemory(&ibd, sizeof(ibd));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = m_IndexCount * sizeof(IndexType);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    // 新建索引缓冲区
    InitData.pSysMem = meshData.indexVec.data();
    device->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf());

    
}


#endif
