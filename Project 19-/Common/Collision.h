//***************************************************************************************
// Collision.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 提供一些封装好的对象和碰撞检测方法
// 注意：WireFrameData目前仍未经过稳定测试，未来有可能会移植到Geometry.h中
// Provide encapsulated collision classes and detection method.
//***************************************************************************************

#pragma once

#ifndef COLLISION_H
#define COLLISION_H

#include <DirectXCollision.h>
#include <vector>
#include "Vertex.h"
#include "Camera.h"


struct Ray
{
	Ray();
	Ray(const DirectX::XMFLOAT3& origin, const DirectX::XMFLOAT3& direction);

	static Ray ScreenToRay(const Camera& camera, float screenX, float screenY);

	bool Hit(const DirectX::BoundingBox& box, float* pOutDist = nullptr, float maxDist = FLT_MAX);
	bool Hit(const DirectX::BoundingOrientedBox& box, float* pOutDist = nullptr, float maxDist = FLT_MAX);
	bool Hit(const DirectX::BoundingSphere& sphere, float* pOutDist = nullptr, float maxDist = FLT_MAX);
	bool XM_CALLCONV Hit(DirectX::FXMVECTOR V0, DirectX::FXMVECTOR V1, DirectX::FXMVECTOR V2, float* pOutDist = nullptr, float maxDist = FLT_MAX);

	DirectX::XMFLOAT3 origin;		// 射线原点
	DirectX::XMFLOAT3 direction;	// 单位方向向量
};


class Collision
{
public:

	// 线框顶点/索引数组
	struct WireFrameData
	{
		std::vector<VertexPosColor> vertexVec;		// 顶点数组
		std::vector<uint32_t> indexVec;				// 索引数组
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
	
    // 视锥体裁剪
    static void XM_CALLCONV FrustumCulling(
        std::vector<Transform>& dest, const std::vector<Transform>& src, 
        const DirectX::BoundingBox& localBox, DirectX::FXMMATRIX View, DirectX::CXMMATRIX Proj);

private:
	static WireFrameData CreateFromCorners(const DirectX::XMFLOAT3(&corners)[8], const DirectX::XMFLOAT4& color);
};





#endif
