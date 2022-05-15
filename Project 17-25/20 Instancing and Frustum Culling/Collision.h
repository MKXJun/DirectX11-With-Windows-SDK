//***************************************************************************************
// Collision.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 提供一些封装好的对象和碰撞检测方法
// 注意：WireFrameData目前仍未经过稳定测试，未来有可能会移植到Geometry.h中
// Provide encapsulated collision classes and detection method.
//***************************************************************************************

#ifndef COLLISION_H
#define COLLISION_H

#include <DirectXCollision.h>
#include <vector>
#include "Vertex.h"
#include "Transform.h"

class Collision
{
public:

    // 线框顶点/索引数组
    struct WireFrameData
    {
        std::vector<VertexPosColor> vertexVec;		// 顶点数组
        std::vector<WORD> indexVec;					// 索引数组
    };

    //
    // 包围盒线框的创建
    //

    // 创建AABB盒线框
    static WireFrameData CreateBoundingBox(const DirectX::BoundingBox& box, const DirectX::XMFLOAT4& color);
    // 创建OBB盒线框
    static WireFrameData CreateBoundingOrientedBox(const DirectX::BoundingOrientedBox& box, const DirectX::XMFLOAT4& color);
    // 创建包围球线框
    static WireFrameData CreateBoundingSphere(const DirectX::BoundingSphere& sphere, const DirectX::XMFLOAT4& color, int slices = 20);
    // 创建视锥体线框
    static WireFrameData CreateBoundingFrustum(const DirectX::BoundingFrustum& frustum, const DirectX::XMFLOAT4& color);


    // 视锥体裁剪
    static std::vector<Transform> XM_CALLCONV FrustumCulling(
        const std::vector<Transform>& transforms, const DirectX::BoundingBox& localBox, DirectX::FXMMATRIX View, DirectX::CXMMATRIX Proj);


private:
    static WireFrameData CreateFromCorners(const DirectX::XMFLOAT3(&corners)[8], const DirectX::XMFLOAT4& color);
};





#endif