#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <Windows.h>
#include <DirectXMath.h>
#include <vector>


class Geometry
{
public:
	struct MeshData
	{
		std::vector<DirectX::XMFLOAT3> posVec;		// 位置数组
		std::vector<DirectX::XMFLOAT3> normalVec;	// 法向量数组
		std::vector<WORD> indexVec;					// 索引数组
	};

	// 创建球体模型数据，levels和slices越大，精度越高。
	static MeshData CreateSphere(float radius = 1.0f, int levels = 20, int slices = 20);

	// 创建立方体模型数据
	static MeshData CreateBox(float width = 2.0f, float height = 2.0f, float depth = 2.0f);

	// 创建圆柱体模型数据，slices越大，精度越高。
	static MeshData CreateCylinder(float radius = 1.0f, float height = 2.0f, int slices = 20);
};



#endif