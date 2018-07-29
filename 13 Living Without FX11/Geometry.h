#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <Windows.h>
#include <vector>
#include "Vertex.h"

class Geometry
{
public:
	struct MeshData
	{
		std::vector<VertexPosNormalTex> vertexVec;	// 顶点数组
		std::vector<WORD> indexVec;					// 索引数组
	};

	// 创建球体模型数据，levels和slices越大，精度越高。
	static MeshData CreateSphere(float radius = 1.0f, int levels = 20, int slices = 20);

	// 创建立方体模型数据
	static MeshData CreateBox(float width = 2.0f, float height = 2.0f, float depth = 2.0f);

	// 创建圆柱体模型数据，slices越大，精度越高。
	static MeshData CreateCylinder(float radius = 1.0f, float height = 2.0f, int slices = 20);

	// 创建一个覆盖NDC屏幕的面
	static MeshData Create2DShow(const DirectX::XMFLOAT2& center = { 0.0f, 0.0f }, const DirectX::XMFLOAT2& scale = { 1.0f, 1.0f });
	static MeshData Create2DShow(float centerX = 0.0f, float centerY = 0.0f, float scaleX = 1.0f, float scaleY = 1.0f);

	// 创建一个平面
	static MeshData CreatePlane(const DirectX::XMFLOAT3& center = {0.0f, 0.0f, 0.0f}, const DirectX::XMFLOAT2& planeSize = { 10.0f, 10.0f }, const DirectX::XMFLOAT2& maxTexCoord = { 1.0f, 1.0f });
	static MeshData CreatePlane(float centerX = 0.0f, float centerY = 0.0f, float centerZ = 0.0f, float width = 10.0f, float depth = 10.0f, float texU = 1.0f, float texV = 1.0f);

};



#endif