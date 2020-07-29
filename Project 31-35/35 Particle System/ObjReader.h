//***************************************************************************************
// ObjReader.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
// Licensed under the MIT License.
//
// - 修正了加载出来的模型是镜像的问题(右手坐标系变换为左手坐标系)
// Modified By X_Jun(MKXJun)
// 2018/9/12 v1.1
// 
// - ObjReader支持通过.obj文件引用.mtl(材质)，并且.mtl(材质)支持引用纹理。
// - 不支持使用/将下一行的内容连接在一起表示一行
// - 不支持索引为负数
// - 不支持使用类似1//2这样的顶点（即不包含纹理坐标的顶点）
// - 对.mtl文件和纹理的引用必须以相对路径的形式提供，且没有支持.和..两种路径格式。
// - 若.mtl材质文件不存在，则内部会使用默认材质值
// - 若.mtl内部没有指定纹理文件引用，需要另外自行加载纹理
// - 要求网格只能以三角形构造
// - .mbo文件是一种二进制文件，用于加快模型加载的速度，内部格式是自定义的
// - .mbo文件已经生成不能随意改变文件位置，若要迁移相关文件需要重新生成.mbo文件
//
// Created By X_Jun(MKXJun)
// 2018/9/9 v1.0
//***************************************************************************************

#ifndef OBJREADER_H
#define OBJREADER_H

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <string>
#include <algorithm>
#include <locale>
#include "Vertex.h"
#include "LightHelper.h"


class MtlReader;

class ObjReader
{
public:
	struct ObjPart
	{
		ObjPart() : material() {}
		~ObjPart() = default;

		Material material;							// 材质
		std::vector<VertexPosNormalTex> vertices;	// 顶点集合
		std::vector<WORD> indices16;				// 顶点数不超过65535时使用
		std::vector<DWORD> indices32;				// 顶点数超过65535时使用
		std::wstring texStrDiffuse;					// 漫射光纹理文件名，需为相对路径，在mbo必须占260字节
	};

	ObjReader() : vMin(), vMax() {}
	~ObjReader() = default;

	// 指定.mbo文件的情况下，若.mbo文件存在，优先读取该文件
	// 否则会读取.obj文件
	// 若.obj文件被读取，且提供了.mbo文件的路径，则会根据已经读取的数据创建.mbo文件
	bool Read(const wchar_t* mboFileName, const wchar_t* objFileName);
	
	bool ReadObj(const wchar_t* objFileName);
	bool ReadMbo(const wchar_t* mboFileName);
	bool WriteMbo(const wchar_t* mboFileName);
public:
	std::vector<ObjPart> objParts;
	DirectX::XMFLOAT3 vMin, vMax;					// AABB盒双顶点
private:
	void AddVertex(const VertexPosNormalTex& vertex, DWORD vpi, DWORD vti, DWORD vni);

	// 缓存有v/vt/vn字符串信息
	std::unordered_map<std::wstring, DWORD> vertexCache;
};

class MtlReader
{
public:
	bool ReadMtl(const wchar_t* mtlFileName);


public:
	std::map<std::wstring, Material> materials;
	std::map<std::wstring, std::wstring> mapKdStrs;
};


#endif

