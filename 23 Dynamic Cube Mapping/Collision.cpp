#include "Collision.h"

using namespace DirectX;

Ray::Ray()
	: origin(), direction(0.0f, 0.0f, 1.0f)
{
}

Ray::Ray(const DirectX::XMFLOAT3 & origin, const DirectX::XMFLOAT3 & direction)
	: origin(origin)
{
	// 射线的direction长度必须为1.0f，误差在10e-5f内
	XMVECTOR dirLength = XMVector3Length(XMLoadFloat3(&direction));
	XMVECTOR error = XMVectorAbs(dirLength - XMVectorSplatOne());
	assert(XMVector3Less(error, XMVectorReplicate(10e-5f)));

	XMStoreFloat3(&this->direction, XMVector3Normalize(XMLoadFloat3(&direction)));
}

Ray Ray::ScreenToRay(const Camera & camera, float screenX, float screenY)
{
	//
	// 节选自DirectX::XMVector3Unproject函数，并省略了从世界坐标系到局部坐标系的变换
	//
	
	// 将屏幕坐标点从视口变换回NDC坐标系
	static const XMVECTORF32 D = { { { -1.0f, 1.0f, 0.0f, 0.0f } } };
	XMVECTOR V = XMVectorSet(screenX, screenY, 0.0f, 1.0f);
	D3D11_VIEWPORT viewPort = camera.GetViewPort();

	XMVECTOR Scale = XMVectorSet(viewPort.Width * 0.5f, -viewPort.Height * 0.5f, viewPort.MaxDepth - viewPort.MinDepth, 1.0f);
	Scale = XMVectorReciprocal(Scale);

	XMVECTOR Offset = XMVectorSet(-viewPort.TopLeftX, -viewPort.TopLeftY, -viewPort.MinDepth, 0.0f);
	Offset = XMVectorMultiplyAdd(Scale, Offset, D.v);

	// 从NDC坐标系变换回世界坐标系
	XMMATRIX Transform = XMMatrixMultiply(camera.GetViewXM(), camera.GetProjXM());
	Transform = XMMatrixInverse(nullptr, Transform);

	XMVECTOR Target = XMVectorMultiplyAdd(V, Scale, Offset);
	Target = XMVector3TransformCoord(Target, Transform);

	// 求出射线
	XMFLOAT3 direction;
	XMStoreFloat3(&direction, XMVector3Normalize(Target - camera.GetPositionXM()));
	return Ray(camera.GetPosition(), direction);
}

bool Ray::Hit(const DirectX::BoundingBox & box, float * pOutDist, float maxDist)
{
	
	float dist;
	bool res = box.Intersects(XMLoadFloat3(&origin), XMLoadFloat3(&direction), dist);
	if (pOutDist)
		*pOutDist = dist;
	return dist > maxDist ? false : res;
}

bool Ray::Hit(const DirectX::BoundingOrientedBox & box, float * pOutDist, float maxDist)
{
	float dist;
	bool res = box.Intersects(XMLoadFloat3(&origin), XMLoadFloat3(&direction), dist);
	if (pOutDist)
		*pOutDist = dist;
	return dist > maxDist ? false : res;
}

bool Ray::Hit(const DirectX::BoundingSphere & sphere, float * pOutDist, float maxDist)
{
	float dist;
	bool res = sphere.Intersects(XMLoadFloat3(&origin), XMLoadFloat3(&direction), dist);
	if (pOutDist)
		*pOutDist = dist;
	return dist > maxDist ? false : res;
}

bool XM_CALLCONV Ray::Hit(FXMVECTOR V0, FXMVECTOR V1, FXMVECTOR V2, float * pOutDist, float maxDist)
{
	float dist;
	bool res = TriangleTests::Intersects(XMLoadFloat3(&origin), XMLoadFloat3(&direction), V0, V1, V2, dist);
	if (pOutDist)
		*pOutDist = dist;
	return dist > maxDist ? false : res;
}



Collision::WireFrameData Collision::CreateBoundingBox(const DirectX::BoundingBox & box, const DirectX::XMFLOAT4 & color)
{
	XMFLOAT3 corners[8];
	box.GetCorners(corners);
	return CreateFromCorners(corners, color);
}

Collision::WireFrameData Collision::CreateBoundingOrientedBox(const DirectX::BoundingOrientedBox & box, const DirectX::XMFLOAT4 & color)
{
	XMFLOAT3 corners[8];
	box.GetCorners(corners);
	return CreateFromCorners(corners, color);
}

Collision::WireFrameData Collision::CreateBoundingSphere(const DirectX::BoundingSphere & sphere, const DirectX::XMFLOAT4 & color, int slices)
{
	WireFrameData data;
	XMVECTOR center = XMLoadFloat3(&sphere.Center), posVec;
	XMFLOAT3 pos;
	float theta = 0.0f;
	for (int i = 0; i < slices; ++i)
	{
		posVec = XMVector3Transform(center + XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), XMMatrixRotationY(theta));
		XMStoreFloat3(&pos, posVec);
		data.vertexVec.push_back({ pos, color });
		posVec = XMVector3Transform(center + XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMMatrixRotationZ(theta));
		XMStoreFloat3(&pos, posVec);
		data.vertexVec.push_back({ pos, color });
		posVec = XMVector3Transform(center + XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f), XMMatrixRotationX(theta));
		XMStoreFloat3(&pos, posVec);
		data.vertexVec.push_back({ pos, color });
		theta += XM_2PI / slices;
	}
	for (int i = 0; i < slices; ++i)
	{
		data.indexVec.push_back(i * 3);
		data.indexVec.push_back((i + 1) % slices * 3);

		data.indexVec.push_back(i * 3 + 1);
		data.indexVec.push_back((i + 1) % slices * 3 + 1);

		data.indexVec.push_back(i * 3 + 2);
		data.indexVec.push_back((i + 1) % slices * 3 + 2);
	}


	return data;
}

Collision::WireFrameData Collision::CreateBoundingFrustum(const DirectX::BoundingFrustum & frustum, const DirectX::XMFLOAT4 & color)
{
	XMFLOAT3 corners[8];
	frustum.GetCorners(corners);
	return CreateFromCorners(corners, color);
}

std::vector<XMMATRIX> XM_CALLCONV Collision::FrustumCulling(
	const std::vector<XMMATRIX>& Matrices,const BoundingBox& localBox, FXMMATRIX View, CXMMATRIX Proj)
{
	std::vector<DirectX::XMMATRIX> acceptedData;

	BoundingFrustum frustum;
	BoundingFrustum::CreateFromMatrix(frustum, Proj);
	XMMATRIX InvView = XMMatrixInverse(nullptr, View);
	// 将视锥体从局部坐标系变换到世界坐标系中
	frustum.Transform(frustum, InvView);

	BoundingOrientedBox localOrientedBox, orientedBox;
	BoundingOrientedBox::CreateFromBoundingBox(localOrientedBox, localBox);
	for (auto& mat : Matrices)
	{
		// 将有向包围盒从局部坐标系变换到世界坐标系中
		localOrientedBox.Transform(orientedBox, mat);
		// 相交检测
		if (frustum.Intersects(orientedBox))
			acceptedData.push_back(mat);
	}

	return acceptedData;
}

std::vector<DirectX::XMMATRIX> XM_CALLCONV Collision::FrustumCulling2(
	const std::vector<DirectX::XMMATRIX>& Matrices,const DirectX::BoundingBox& localBox, DirectX::FXMMATRIX View, DirectX::CXMMATRIX Proj)
{
	std::vector<DirectX::XMMATRIX> acceptedData;

	BoundingFrustum frustum, localFrustum;
	BoundingFrustum::CreateFromMatrix(frustum, Proj);
	XMMATRIX InvView = XMMatrixInverse(nullptr, View);
	for (auto& mat : Matrices)
	{
		XMMATRIX InvWorld = XMMatrixInverse(nullptr, mat);

		// 将视锥体从观察坐标系(或局部坐标系)变换到物体所在的局部坐标系中
		frustum.Transform(localFrustum, InvView * InvWorld);
		// 相交检测
		if (localFrustum.Intersects(localBox))
			acceptedData.push_back(mat);
	}

	return acceptedData;
}

std::vector<DirectX::XMMATRIX> XM_CALLCONV Collision::FrustumCulling3(
	const std::vector<DirectX::XMMATRIX>& Matrices,const DirectX::BoundingBox& localBox, DirectX::FXMMATRIX View, DirectX::CXMMATRIX Proj)
{
	std::vector<DirectX::XMMATRIX> acceptedData;

	BoundingFrustum frustum;
	BoundingFrustum::CreateFromMatrix(frustum, Proj);

	BoundingOrientedBox localOrientedBox, orientedBox;
	BoundingOrientedBox::CreateFromBoundingBox(localOrientedBox, localBox);
	for (auto& mat : Matrices)
	{
		// 将有向包围盒从局部坐标系变换到视锥体所在的局部坐标系(观察坐标系)中
		localOrientedBox.Transform(orientedBox, mat * View);
		// 相交检测
		if (frustum.Intersects(orientedBox))
			acceptedData.push_back(mat);
	}

	return acceptedData;
}

Collision::WireFrameData Collision::CreateFromCorners(const DirectX::XMFLOAT3(&corners)[8], const DirectX::XMFLOAT4 & color)
{
	WireFrameData data;
	// AABB/OBB顶点索引如下    视锥体顶点索引如下
	//     3_______2             4__________5
	//    /|      /|             |\        /|
	//  7/_|____6/ |             | \      / |
	//  |  |____|__|            7|_0\____/1_|6
	//  | /0    | /1              \ |    | /
	//  |/______|/                 \|____|/
	//  4       5                   3     2
	for (int i = 0; i < 8; ++i)
		data.vertexVec.push_back({ corners[i], color });
	for (int i = 0; i < 4; ++i)
	{
		data.indexVec.push_back(i);
		data.indexVec.push_back(i);

		data.indexVec.push_back(i);
		data.indexVec.push_back((i + 1) % 4);

		data.indexVec.push_back(i + 4);
		data.indexVec.push_back((i + 1) % 4 + 4);
	}
	return data;
}

