#include "ObjReader.h"

using namespace DirectX;
bool ObjReader::Read(const wchar_t* mboFileName, const wchar_t* objFileName)
{
	if (mboFileName && ReadMbo(mboFileName))
	{
		return true;
	}
	else if (objFileName)
	{
		bool status = ReadObj(objFileName);
		if (status && mboFileName)
			return WriteMbo(mboFileName);
		return status;
	}

	return false;
}

bool ObjReader::ReadObj(const wchar_t* objFileName)
{
	objParts.clear();
	vertexCache.clear();

	MtlReader mtlReader;

	std::vector<XMFLOAT3>   positions;
	std::vector<XMFLOAT3>   normals;
	std::vector<XMFLOAT2>   texCoords;

	XMVECTOR vecMin = g_XMInfinity, vecMax = g_XMNegInfinity;

	std::wifstream wfin(objFileName);
	if (!wfin.is_open())
		return false;

	// 切换中文
	std::locale china("chs");
	china = wfin.imbue(china);
	for (;;)
	{
		std::wstring wstr;
		if (!(wfin >> wstr))
			break;

		if (wstr[0] == '#')
		{
			//
			// 忽略注释所在行
			//
			while (!wfin.eof() && wfin.get() != '\n')
				continue;
		}
		else if (wstr == L"o" || wstr == L"g")
		{
			// 
			// 对象名(组名)
			//
			objParts.emplace_back(ObjPart());
			// 提供默认材质
			objParts.back().material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
			objParts.back().material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
			objParts.back().material.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

			vertexCache.clear();
		}
		else if (wstr == L"v")
		{
			//
			// 顶点位置
			//

			// 注意obj使用的是右手坐标系，而不是左手坐标系
			// 需要将z值反转
			XMFLOAT3 pos;
			wfin >> pos.x >> pos.y >> pos.z;
			pos.z = -pos.z;
			positions.push_back(pos);
			XMVECTOR vecPos = XMLoadFloat3(&pos);
			vecMax = XMVectorMax(vecMax, vecPos);
			vecMin = XMVectorMin(vecMin, vecPos);
		}
		else if (wstr == L"vt")
		{
			//
			// 顶点纹理坐标
			//

			// 注意obj使用的是笛卡尔坐标系，而不是纹理坐标系
			float u, v;
			wfin >> u >> v;
			v = 1.0f - v;
			texCoords.emplace_back(XMFLOAT2(u, v));
		}
		else if (wstr == L"vn")
		{
			//
			// 顶点法向量
			//

			// 注意obj使用的是右手坐标系，而不是左手坐标系
			// 需要将z值反转
			float x, y, z;
			wfin >> x >> y >> z;
			z = -z;
			normals.emplace_back(XMFLOAT3(x, y, z));
		}
		else if (wstr == L"mtllib")
		{
			//
			// 指定某一文件的材质
			//
			std::wstring mtlFile;
			wfin >> mtlFile;
			// 去掉前后空格
			size_t beg = 0, ed = mtlFile.size();
			while (iswspace(mtlFile[beg]))
				beg++;
			while (ed > beg&& iswspace(mtlFile[ed - 1]))
				ed--;
			mtlFile = mtlFile.substr(beg, ed - beg);
			// 获取路径
			std::wstring dir = objFileName;
			size_t pos;
			if ((pos = dir.find_last_of('/')) == std::wstring::npos &&
				(pos = dir.find_last_of('\\')) == std::wstring::npos)
			{
				pos = 0;
			}
			else
			{
				pos += 1;
			}


			mtlReader.ReadMtl((dir.erase(pos) + mtlFile).c_str());
		}
		else if (wstr == L"usemtl")
		{
			//
			// 使用之前指定文件内部的某一材质
			//
			std::wstring mtlName;
			std::getline(wfin, mtlName);
			// 去掉前后空格
			size_t beg = 0, ed = mtlName.size();
			while (iswspace(mtlName[beg]))
				beg++;
			while (ed > beg&& iswspace(mtlName[ed - 1]))
				ed--;
			mtlName = mtlName.substr(beg, ed - beg);

			objParts.back().material = mtlReader.materials[mtlName];
			objParts.back().texStrDiffuse = mtlReader.mapKdStrs[mtlName];
		}
		else if (wstr == L"f")
		{
			//
			// 几何面
			//
			VertexPosNormalTex vertex;
			DWORD vpi[3], vni[3], vti[3];
			wchar_t ignore;

			// 顶点位置索引/纹理坐标索引/法向量索引
			// 原来右手坐标系下顶点顺序是逆时针排布
			// 现在需要转变为左手坐标系就需要将三角形顶点反过来输入
			for (int i = 2; i >= 0; --i)
			{
				wfin >> vpi[i] >> ignore >> vti[i] >> ignore >> vni[i];
			}

			for (int i = 0; i < 3; ++i)
			{
				vertex.pos = positions[vpi[i] - 1];
				vertex.normal = normals[vni[i] - 1];
				vertex.tex = texCoords[vti[i] - 1];
				AddVertex(vertex, vpi[i], vti[i], vni[i]);
			}


			while (iswblank(wfin.peek()))
				wfin.get();
			// 几何面顶点数可能超过了3，不支持该格式
			if (wfin.peek() != '\n')
				return false;
		}
	}

	// 顶点数不超过WORD的最大值的话就使用16位WORD存储
	for (auto& part : objParts)
	{
		if (part.vertices.size() < 65535)
		{
			for (auto& i : part.indices32)
			{
				part.indices16.push_back((WORD)i);
			}
			part.indices32.clear();
		}
	}

	XMStoreFloat3(&vMax, vecMax);
	XMStoreFloat3(&vMin, vecMin);

	return true;
}

bool ObjReader::ReadMbo(const wchar_t* mboFileName)
{
	// [Part数目] 4字节
	// [AABB盒顶点vMax] 12字节
	// [AABB盒顶点vMin] 12字节
	// [Part
	//   [漫射光材质文件名]520字节
	//   [材质]64字节
	//   [顶点数]4字节
	//   [索引数]4字节
	//   [顶点]32*顶点数 字节
	//   [索引]2(或4)*索引数 字节，取决于顶点数是否不超过65535
	// ]
	// ...
	std::ifstream fin(mboFileName, std::ios::in | std::ios::binary);
	if (!fin.is_open())
		return false;

	UINT parts = (UINT)objParts.size();
	// [Part数目] 4字节
	fin.read(reinterpret_cast<char*>(&parts), sizeof(UINT));
	objParts.resize(parts);

	// [AABB盒顶点vMax] 12字节
	fin.read(reinterpret_cast<char*>(&vMax), sizeof(XMFLOAT3));
	// [AABB盒顶点vMin] 12字节
	fin.read(reinterpret_cast<char*>(&vMin), sizeof(XMFLOAT3));


	for (UINT i = 0; i < parts; ++i)
	{
		wchar_t filePath[MAX_PATH];
		// [漫射光材质文件名]520字节
		fin.read(reinterpret_cast<char*>(filePath), MAX_PATH * sizeof(wchar_t));
		objParts[i].texStrDiffuse = filePath;
		// [材质]64字节
		fin.read(reinterpret_cast<char*>(&objParts[i].material), sizeof(Material));
		UINT vertexCount, indexCount;
		// [顶点数]4字节
		fin.read(reinterpret_cast<char*>(&vertexCount), sizeof(UINT));
		// [索引数]4字节
		fin.read(reinterpret_cast<char*>(&indexCount), sizeof(UINT));
		// [顶点]32*顶点数 字节
		objParts[i].vertices.resize(vertexCount);
		fin.read(reinterpret_cast<char*>(objParts[i].vertices.data()), vertexCount * sizeof(VertexPosNormalTex));

		if (vertexCount > 65535)
		{
			// [索引]4*索引数 字节
			objParts[i].indices32.resize(indexCount);
			fin.read(reinterpret_cast<char*>(objParts[i].indices32.data()), indexCount * sizeof(DWORD));
		}
		else
		{
			// [索引]2*索引数 字节
			objParts[i].indices16.resize(indexCount);
			fin.read(reinterpret_cast<char*>(objParts[i].indices16.data()), indexCount * sizeof(WORD));
		}
	}

	fin.close();

	return true;
}

bool ObjReader::WriteMbo(const wchar_t* mboFileName)
{
	// [Part数目] 4字节
	// [AABB盒顶点vMax] 12字节
	// [AABB盒顶点vMin] 12字节
	// [Part
	//   [环境光材质文件名]520字节
	//   [漫射光材质文件名]520字节
	//   [材质]64字节
	//   [顶点数]4字节
	//   [索引数]4字节
	//   [顶点]32*顶点数 字节
	//   [索引]2(或4)*索引数 字节，取决于顶点数是否不超过65535
	// ]
	// ...
	std::ofstream fout(mboFileName, std::ios::out | std::ios::binary);
	UINT parts = (UINT)objParts.size();
	// [Part数目] 4字节
	fout.write(reinterpret_cast<const char*>(&parts), sizeof(UINT));

	// [AABB盒顶点vMax] 12字节
	fout.write(reinterpret_cast<const char*>(&vMax), sizeof(XMFLOAT3));
	// [AABB盒顶点vMin] 12字节
	fout.write(reinterpret_cast<const char*>(&vMin), sizeof(XMFLOAT3));

	// [Part
	for (UINT i = 0; i < parts; ++i)
	{
		wchar_t filePath[MAX_PATH];
		wcscpy_s(filePath, objParts[i].texStrDiffuse.c_str());
		// [漫射光材质文件名]520字节
		fout.write(reinterpret_cast<const char*>(filePath), MAX_PATH * sizeof(wchar_t));
		// [材质]64字节
		fout.write(reinterpret_cast<const char*>(&objParts[i].material), sizeof(Material));
		UINT vertexCount = (UINT)objParts[i].vertices.size();
		// [顶点数]4字节
		fout.write(reinterpret_cast<const char*>(&vertexCount), sizeof(UINT));

		UINT indexCount;
		if (vertexCount > 65535)
		{
			indexCount = (UINT)objParts[i].indices32.size();
			// [索引数]4字节
			fout.write(reinterpret_cast<const char*>(&indexCount), sizeof(UINT));
			// [顶点]32*顶点数 字节
			fout.write(reinterpret_cast<const char*>(objParts[i].vertices.data()), vertexCount * sizeof(VertexPosNormalTex));
			// [索引]4*索引数 字节
			fout.write(reinterpret_cast<const char*>(objParts[i].indices32.data()), indexCount * sizeof(DWORD));
		}
		else
		{
			indexCount = (UINT)objParts[i].indices16.size();
			// [索引数]4字节
			fout.write(reinterpret_cast<const char*>(&indexCount), sizeof(UINT));
			// [顶点]32*顶点数 字节
			fout.write(reinterpret_cast<const char*>(objParts[i].vertices.data()), vertexCount * sizeof(VertexPosNormalTex));
			// [索引]2*索引数 字节
			fout.write(reinterpret_cast<const char*>(objParts[i].indices16.data()), indexCount * sizeof(WORD));
		}
	}
	// ]
	fout.close();

	return true;
}

void ObjReader::AddVertex(const VertexPosNormalTex& vertex, DWORD vpi, DWORD vti, DWORD vni)
{
	std::wstring idxStr = std::to_wstring(vpi) + L"/" + std::to_wstring(vti) + L"/" + std::to_wstring(vni);

	// 寻找是否有重复顶点
	auto it = vertexCache.find(idxStr);
	if (it != vertexCache.end())
	{
		objParts.back().indices32.push_back(it->second);
	}
	else
	{
		objParts.back().vertices.push_back(vertex);
		DWORD pos = (DWORD)objParts.back().vertices.size() - 1;
		vertexCache[idxStr] = pos;
		objParts.back().indices32.push_back(pos);
	}
}



bool MtlReader::ReadMtl(const wchar_t* mtlFileName)
{
	materials.clear();
	mapKdStrs.clear();


	std::wifstream wfin(mtlFileName);
	std::locale china("chs");
	china = wfin.imbue(china);


	if (!wfin.is_open())
		return false;

	std::wstring wstr;
	std::wstring currMtl;
	for (;;)
	{
		if (!(wfin >> wstr))
			break;

		if (wstr[0] == '#')
		{
			//
			// 忽略注释所在行
			//
			while (wfin.get() != '\n')
				continue;
		}
		else if (wstr == L"newmtl")
		{
			//
			// 新材质
			//

			std::getline(wfin, currMtl);
			// 去掉前后空格
			size_t beg = 0, ed = currMtl.size();
			while (iswspace(currMtl[beg]))
				beg++;
			while (ed > beg&& iswspace(currMtl[ed - 1]))
				ed--;
			currMtl = currMtl.substr(beg, ed - beg);
		}
		else if (wstr == L"Ka")
		{
			//
			// 环境光反射颜色
			//
			XMFLOAT4& ambient = materials[currMtl].ambient;
			wfin >> ambient.x >> ambient.y >> ambient.z;
			if (ambient.w == 0.0f)
				ambient.w = 1.0f;
		}
		else if (wstr == L"Kd")
		{
			//
			// 漫射光反射颜色
			//
			XMFLOAT4& diffuse = materials[currMtl].diffuse;
			wfin >> diffuse.x >> diffuse.y >> diffuse.z;
			if (diffuse.w == 0.0f)
				diffuse.w = 1.0f;
		}
		else if (wstr == L"Ks")
		{
			//
			// 镜面光反射颜色
			//
			XMFLOAT4& specular = materials[currMtl].specular;
			wfin >> specular.x >> specular.y >> specular.z;
		}
		else if (wstr == L"Ns")
		{
			//
			// 镜面系数
			//
			wfin >> materials[currMtl].specular.w;
		}
		else if (wstr == L"d" || wstr == L"Tr")
		{
			//
			// d为不透明度 Tr为透明度
			//
			float alpha;
			wfin >> alpha;
			if (wstr == L"Tr")
				alpha = 1.0f - alpha;
			materials[currMtl].ambient.w = alpha;
			materials[currMtl].diffuse.w = alpha;
		}
		else if (wstr == L"map_Kd")
		{
			//
			// map_Kd为漫反射使用的纹理
			//
			std::wstring fileName;
			std::getline(wfin, fileName);
			// 去掉前后空格
			size_t beg = 0, ed = fileName.size();
			while (iswspace(fileName[beg]))
				beg++;
			while (ed > beg&& iswspace(fileName[ed - 1]))
				ed--;
			fileName = fileName.substr(beg, ed - beg);

			// 追加路径
			std::wstring dir = mtlFileName;
			size_t pos;
			if ((pos = dir.find_last_of('/')) == std::wstring::npos &&
				(pos = dir.find_last_of('\\')) == std::wstring::npos)
				pos = 0;
			else
				pos += 1;

			mapKdStrs[currMtl] = dir.erase(pos) + fileName;
		}
	}

	return true;
}

