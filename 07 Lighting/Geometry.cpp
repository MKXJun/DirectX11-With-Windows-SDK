#include "Geometry.h"

using namespace DirectX;

Geometry::MeshData Geometry::CreateSphere(float radius, int levels, int slices)
{
	MeshData meshData;
	float phi = 0.0f, theta = 0.0f;
	float per_phi = XM_PI / levels;
	float per_theta = XM_2PI / slices;
	float x, y, z;

	// 放入顶端点
	meshData.posVec.push_back(XMFLOAT3(0.0f, radius, 0.0f));
	meshData.normalVec.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
	for (int i = 1; i < levels; ++i)
	{
		phi = per_phi * i;
		for (int j = 0; j < slices; ++j)
		{
			theta = per_theta * j;
			x = radius * sinf(phi) * cosf(theta);
			y = radius * cosf(phi);
			z = radius * sinf(phi) * sinf(theta);
			// 计算出局部坐标和法向量
			XMFLOAT3 pos = XMFLOAT3(x, y, z), normal;
			meshData.posVec.push_back(pos);
			XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&pos)));
			meshData.normalVec.push_back(normal);

		}
	}
	// 放入底端点
	meshData.posVec.push_back(XMFLOAT3(0.0f, -radius, 0.0f));
	meshData.normalVec.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));

	// 逐渐放入索引
	if (levels > 1)
	{
		for (int j = 1; j <= slices; ++j)
		{
			meshData.indexVec.push_back(0);
			meshData.indexVec.push_back(j % slices + 1);
			meshData.indexVec.push_back(j);
			
		}
	}


	for (int i = 1; i < levels - 1; ++i)
	{
		for (int j = 1; j <= slices; ++j)
		{

			meshData.indexVec.push_back((i - 1) * slices + j);
			meshData.indexVec.push_back((i - 1) * slices + j % slices + 1);
			meshData.indexVec.push_back(i * slices + j % slices + 1);

			meshData.indexVec.push_back(i * slices + j % slices + 1);
			meshData.indexVec.push_back(i * slices + j);
			meshData.indexVec.push_back((i - 1) * slices + j);

		}

	}

	// 逐渐放入索引
	if (levels > 1)
	{
		for (int j = 1; j <= slices; ++j)
		{
			meshData.indexVec.push_back((levels - 2) * slices + j);
			meshData.indexVec.push_back((levels - 2) * slices + j % slices + 1);
			meshData.indexVec.push_back((WORD)(meshData.posVec.size() - 1));
		}
	}
	

	return meshData;
}

Geometry::MeshData Geometry::CreateBox(float width, float height, float depth)
{
	MeshData meshData;
	float w2 = width / 2, h2 = height / 2, d2 = depth / 2;

	meshData.posVec.resize(24);
	// 顶面
	meshData.posVec[0] = XMFLOAT3(-w2, h2, -d2);
	meshData.posVec[1] = XMFLOAT3(-w2, h2, d2);
	meshData.posVec[2] = XMFLOAT3(w2, h2, d2);
	meshData.posVec[3] = XMFLOAT3(w2, h2, -d2);
	// 底面
	meshData.posVec[4] = XMFLOAT3(w2, -h2, -d2);
	meshData.posVec[5] = XMFLOAT3(w2, -h2, d2);
	meshData.posVec[6] = XMFLOAT3(-w2, -h2, d2);
	meshData.posVec[7] = XMFLOAT3(-w2, -h2, -d2);
	// 左面
	meshData.posVec[8] = XMFLOAT3(-w2, -h2, d2);
	meshData.posVec[9] = XMFLOAT3(-w2, h2, d2);
	meshData.posVec[10] = XMFLOAT3(-w2, h2, -d2);
	meshData.posVec[11] = XMFLOAT3(-w2, -h2, -d2);
	// 右面
	meshData.posVec[12] = XMFLOAT3(w2, -h2, -d2);
	meshData.posVec[13] = XMFLOAT3(w2, h2, -d2);
	meshData.posVec[14] = XMFLOAT3(w2, h2, d2);
	meshData.posVec[15] = XMFLOAT3(w2, -h2, d2);
	// 背面
	meshData.posVec[16] = XMFLOAT3(w2, -h2, d2);
	meshData.posVec[17] = XMFLOAT3(w2, h2, d2);
	meshData.posVec[18] = XMFLOAT3(-w2, h2, d2);
	meshData.posVec[19] = XMFLOAT3(-w2, -h2, d2);
	// 正面
	meshData.posVec[20] = XMFLOAT3(-w2, -h2, -d2);
	meshData.posVec[21] = XMFLOAT3(-w2, h2, -d2);
	meshData.posVec[22] = XMFLOAT3(w2, h2, -d2);
	meshData.posVec[23] = XMFLOAT3(w2, -h2, -d2);

	meshData.normalVec.resize(24);
	for (int i = 0; i < 4; ++i)
	{
		meshData.normalVec[i] = XMFLOAT3(0.0f, 1.0f, 0.0f);			// 顶面
		meshData.normalVec[i + 4] = XMFLOAT3(0.0f, -1.0f, 0.0f);	// 底面
		meshData.normalVec[i + 8] = XMFLOAT3(-1.0f, 0.0f, 0.0f);	// 左面
		meshData.normalVec[i + 12] = XMFLOAT3(1.0f, 0.0f, 0.0f);	// 右面
		meshData.normalVec[i + 16] = XMFLOAT3(0.0f, 0.0f, 1.0f);	// 背面
		meshData.normalVec[i + 20] = XMFLOAT3(0.0f, 0.0f, -1.0f);	// 正面
	}
	
	meshData.indexVec = {
		0, 1, 2, 2, 3, 0,		// 顶面
		4, 5, 6, 6, 7, 4,		// 底面
		8, 9, 10, 10, 11, 8,	// 左面
		12, 13, 14, 14, 15, 12,	// 右面
		16, 17, 18, 18, 19, 16, // 背面
		20, 21, 22, 22, 23, 20	// 正面
	};

	return meshData;
}

Geometry::MeshData Geometry::CreateCylinder(float radius, float height, int slices)
{
	MeshData meshData = CreateCylinderNoCap(radius, height, slices);
	float h2 = height / 2;
	float theta = 0.0f;
	float per_theta = XM_2PI / slices;

	int offset = 2 * (slices + 1);
	// 放入顶端圆心
	meshData.posVec.push_back(XMFLOAT3(0.0f, h2, 0.0f));
	meshData.normalVec.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
	// 放入顶端圆上各点
	for (int i = 0; i < slices; ++i)
	{
		theta = i * per_theta;
		meshData.posVec.push_back(XMFLOAT3(cosf(theta), h2, sinf(theta)));
		meshData.normalVec.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
	}
	// 逐渐放入索引
	for (int i = 1; i <= slices; ++i)
	{
		meshData.indexVec.push_back(offset);
		meshData.indexVec.push_back(offset + i % slices + 1);
		meshData.indexVec.push_back(offset + i);
	}


	// 放入底端圆心
	meshData.posVec.push_back(XMFLOAT3(0.0f, -h2, 0.0f));
	meshData.normalVec.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));
	// 放入底部圆上各点
	for (int i = 0; i < slices; ++i)
	{
		theta = i * per_theta;
		meshData.posVec.push_back(XMFLOAT3(cosf(theta), -h2, sinf(theta)));
		meshData.normalVec.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));

	}
	// 逐渐放入索引
	offset += slices + 1;
	for (int i = 1; i <= slices; ++i)
	{
		meshData.indexVec.push_back(offset);
		meshData.indexVec.push_back(offset + i);
		meshData.indexVec.push_back(offset + i % slices + 1);
	}

	return meshData;
}

Geometry::MeshData Geometry::CreateCylinderNoCap(float radius, float height, int slices)
{
	MeshData meshData;
	float h2 = height / 2;
	float theta = 0.0f;
	float per_theta = XM_2PI / slices;

	// 放入侧面顶端点
	for (int i = 0; i <= slices; ++i)
	{
		theta = i * per_theta;
		meshData.posVec.push_back(XMFLOAT3(radius * cosf(theta), h2, radius * sinf(theta)));
		meshData.normalVec.push_back(XMFLOAT3(cosf(theta), 0.0f, sinf(theta)));
	}
	// 放入侧面底端点
	for (int i = 0; i <= slices; ++i)
	{
		theta = i * per_theta;
		meshData.posVec.push_back(XMFLOAT3(radius * cosf(theta), -h2, radius * sinf(theta)));
		meshData.normalVec.push_back(XMFLOAT3(cosf(theta), 0.0f, sinf(theta)));
	}

	// 放入索引
	for (int i = 0; i < slices; ++i)
	{
		meshData.indexVec.push_back(i);
		meshData.indexVec.push_back(i + 1);
		meshData.indexVec.push_back((slices + 1) + i + 1);

		meshData.indexVec.push_back((slices + 1) + i + 1);
		meshData.indexVec.push_back((slices + 1) + i);
		meshData.indexVec.push_back(i);
	}

	return meshData;
}