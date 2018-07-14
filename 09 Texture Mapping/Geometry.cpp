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
	meshData.vertexVec.push_back({ XMFLOAT3(0.0f, radius, 0.0f) , XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) });


	for (int i = 1; i < levels; ++i)
	{
		phi = per_phi * i;
		for (int j = 0; j < slices; ++j)
		{
			theta = per_theta * j;
			x = radius * sinf(phi) * cosf(theta);
			y = radius * cosf(phi);
			z = radius * sinf(phi) * sinf(theta);
			// 计算出局部坐标、法向量和纹理坐标
			XMFLOAT3 pos = XMFLOAT3(x, y, z), normal;
			XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&pos)));
			meshData.vertexVec.push_back({pos, normal, XMFLOAT2(theta / XM_2PI, phi / XM_PI)});
		}
	}
	// 放入底端点
	meshData.vertexVec.push_back({ XMFLOAT3(0.0f, -radius, 0.0f) , XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) });

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
			meshData.indexVec.push_back((WORD)(meshData.vertexVec.size() - 1));
		}
	}
	

	return meshData;
}

Geometry::MeshData Geometry::CreateBox(float width, float height, float depth)
{
	MeshData meshData;
	float w2 = width / 2, h2 = height / 2, d2 = depth / 2;

	meshData.vertexVec.resize(24);
	// 顶面
	meshData.vertexVec[0].pos = XMFLOAT3(-w2, h2, -d2);
	meshData.vertexVec[1].pos = XMFLOAT3(-w2, h2, d2);
	meshData.vertexVec[2].pos = XMFLOAT3(w2, h2, d2);
	meshData.vertexVec[3].pos = XMFLOAT3(w2, h2, -d2);
	// 底面
	meshData.vertexVec[4].pos = XMFLOAT3(w2, -h2, -d2);
	meshData.vertexVec[5].pos = XMFLOAT3(w2, -h2, d2);
	meshData.vertexVec[6].pos = XMFLOAT3(-w2, -h2, d2);
	meshData.vertexVec[7].pos = XMFLOAT3(-w2, -h2, -d2);
	// 左面
	meshData.vertexVec[8].pos = XMFLOAT3(-w2, -h2, d2);
	meshData.vertexVec[9].pos = XMFLOAT3(-w2, h2, d2);
	meshData.vertexVec[10].pos = XMFLOAT3(-w2, h2, -d2);
	meshData.vertexVec[11].pos = XMFLOAT3(-w2, -h2, -d2);
	// 右面
	meshData.vertexVec[12].pos = XMFLOAT3(w2, -h2, -d2);
	meshData.vertexVec[13].pos = XMFLOAT3(w2, h2, -d2);
	meshData.vertexVec[14].pos = XMFLOAT3(w2, h2, d2);
	meshData.vertexVec[15].pos = XMFLOAT3(w2, -h2, d2);
	// 背面
	meshData.vertexVec[16].pos = XMFLOAT3(w2, -h2, d2);
	meshData.vertexVec[17].pos = XMFLOAT3(w2, h2, d2);
	meshData.vertexVec[18].pos = XMFLOAT3(-w2, h2, d2);
	meshData.vertexVec[19].pos = XMFLOAT3(-w2, -h2, d2);
	// 正面
	meshData.vertexVec[20].pos = XMFLOAT3(-w2, -h2, -d2);
	meshData.vertexVec[21].pos = XMFLOAT3(-w2, h2, -d2);
	meshData.vertexVec[22].pos = XMFLOAT3(w2, h2, -d2);
	meshData.vertexVec[23].pos = XMFLOAT3(w2, -h2, -d2);

	for (int i = 0; i < 4; ++i)
	{
		meshData.vertexVec[i].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);		// 顶面
		meshData.vertexVec[i + 4].normal = XMFLOAT3(0.0f, -1.0f, 0.0f);	// 底面
		meshData.vertexVec[i + 8].normal = XMFLOAT3(-1.0f, 0.0f, 0.0f);	// 左面
		meshData.vertexVec[i + 12].normal = XMFLOAT3(1.0f, 0.0f, 0.0f);	// 右面
		meshData.vertexVec[i + 16].normal = XMFLOAT3(0.0f, 0.0f, 1.0f);	// 背面
		meshData.vertexVec[i + 20].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);// 正面
	}

	for (int i = 0; i < 6; ++i)
	{
		meshData.vertexVec[i * 4].tex = XMFLOAT2(0.0f, 1.0f);
		meshData.vertexVec[i * 4 + 1].tex = XMFLOAT2(0.0f, 0.0f);
		meshData.vertexVec[i * 4 + 2].tex = XMFLOAT2(1.0f, 0.0f);
		meshData.vertexVec[i * 4 + 3].tex = XMFLOAT2(1.0f, 1.0f);
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
	MeshData meshData;
	float h2 = height / 2;
	float theta = 0.0f;
	float per_theta = XM_2PI / slices;
	// 放入顶端圆心
	meshData.vertexVec.push_back({ XMFLOAT3(0.0f, h2, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.5f, 0.5f)});
	// 放入顶端圆上各点
	for (int i = 0; i < slices; ++i)
	{
		theta = i * per_theta;
		meshData.vertexVec.push_back({ XMFLOAT3(cosf(theta), h2, sinf(theta)), XMFLOAT3(0.0f, 1.0f, 0.0f), 
			XMFLOAT2(cosf(theta) / 2 + 0.5f, sinf(theta) / 2 + 0.5f) });
	}
	// 逐渐放入索引
	for (int i = 1; i <= slices; ++i)
	{
		meshData.indexVec.push_back(0);
		meshData.indexVec.push_back(i % slices + 1);
		meshData.indexVec.push_back(i);
	}

	// 放入侧面顶端点
	for (int i = 0; i < slices; ++i)
	{
		theta = i * per_theta;
		meshData.vertexVec.push_back({ XMFLOAT3(cosf(theta), h2, sinf(theta)), XMFLOAT3(cosf(theta), 0.0f, sinf(theta)),
			XMFLOAT2(theta / XM_2PI, 0.0f) });
	}
	// 放入侧面底端点
	for (int i = 0; i < slices; ++i)
	{
		theta = i * per_theta;
		meshData.vertexVec.push_back({ XMFLOAT3(cosf(theta), -h2, sinf(theta)), XMFLOAT3(cosf(theta), 0.0f, sinf(theta)),
			XMFLOAT2(theta / XM_2PI, 1.0f) });
	}
	// 放入索引
	for (int i = 1; i <= slices; ++i)
	{
		meshData.indexVec.push_back(slices + i);
		meshData.indexVec.push_back(slices + i % slices + 1);
		meshData.indexVec.push_back(2 * slices + i % slices + 1);

		meshData.indexVec.push_back(2 * slices + i % slices + 1);
		meshData.indexVec.push_back(2 * slices + i);
		meshData.indexVec.push_back(slices + i);
	}
	// 放入底部圆上各点
	for (int i = 0; i < slices; ++i)
	{
		theta = i * per_theta;
		meshData.vertexVec.push_back({ XMFLOAT3(cosf(theta), -h2, sinf(theta)), XMFLOAT3(0.0f, -1.0f, 0.0f),
			XMFLOAT2(cosf(theta) / 2 + 0.5f, sinf(theta) / 2 + 0.5f) });
	}
	// 放入底端圆心
	meshData.vertexVec.push_back({ XMFLOAT3(0.0f, -h2, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.5f, 0.5f) });

	// 逐渐放入索引
	WORD numPos = (WORD)meshData.vertexVec.size() - 1;
	for (int i = 1; i <= slices; ++i)
	{
		meshData.indexVec.push_back(numPos);
		meshData.indexVec.push_back(3 * slices + i);
		meshData.indexVec.push_back(3 * slices + i % slices + 1);
	}
	
	return meshData;
}

Geometry::MeshData Geometry::Create2DShow(float centerX, float centerY, float scaleX, float scaleY)
{
	MeshData meshData;


	meshData.vertexVec.push_back({ XMFLOAT3(centerX - scaleX, centerY - scaleY, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) });
	meshData.vertexVec.push_back({ XMFLOAT3(centerX - scaleX, centerY + scaleY, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) });
	meshData.vertexVec.push_back({ XMFLOAT3(centerX + scaleX, centerY + scaleY, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) });
	meshData.vertexVec.push_back({ XMFLOAT3(centerX + scaleX, centerY - scaleY, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) });

	meshData.indexVec = { 0, 1, 2, 2, 3, 0 };
	return meshData;
}
