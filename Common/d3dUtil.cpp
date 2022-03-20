#include "d3dUtil.h"
#include "DDSTextureLoader.h"

using namespace DirectX;

std::wstring UTF8ToWString(std::string_view utf8str)
{
	if (utf8str.empty()) return std::wstring();
	int cbMultiByte = static_cast<int>(utf8str.size());
	int req = MultiByteToWideChar(CP_UTF8, 0, utf8str.data(), cbMultiByte, nullptr, 0);
	std::wstring res(req, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8str.data(), cbMultiByte, &res[0], req);
	return res;
}

std::string WStringToUTF8(std::wstring_view wstr)
{
	if (wstr.empty()) return std::string();
	int cbMultiByte = static_cast<int>(wstr.size());
	int req = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), cbMultiByte, nullptr, 0, nullptr, nullptr);
	std::string res(req, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), cbMultiByte, &res[0], req, nullptr, nullptr);
	return res;
}

size_t StringToID(std::string_view str)
{
	static std::hash<std::string_view> hash;
	return hash(str);
}

//
// 着色器编译相关函数
//

HRESULT CreateShaderFromFile(
	const WCHAR* csoFileNameInOut,
	const WCHAR* hlslFileName,
	const D3D_SHADER_MACRO* pDefines,
	LPCSTR entryPoint,
	LPCSTR shaderModel,
	ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	// 寻找是否有已经编译好的顶点着色器
	if (csoFileNameInOut && D3DReadFileToBlob(csoFileNameInOut, ppBlobOut) == S_OK)
	{
		return hr;
	}
	else
	{
		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		// 设置 D3DCOMPILE_DEBUG 标志用于获取着色器调试信息。该标志可以提升调试体验，
		// 但仍然允许着色器进行优化操作
		dwShaderFlags |= D3DCOMPILE_DEBUG;

		// 在Debug环境下禁用优化以避免出现一些不合理的情况
		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ID3DBlob* errorBlob = nullptr;
		hr = D3DCompileFromFile(hlslFileName, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, shaderModel,
			dwShaderFlags, 0, ppBlobOut, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob != nullptr)
			{
				OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			}
			SAFE_RELEASE(errorBlob);
			return hr;
		}

		// 若指定了输出文件名，则将着色器二进制信息输出
		if (csoFileNameInOut)
		{
			return D3DWriteBlobToFile(*ppBlobOut, csoFileNameInOut, FALSE);
		}
	}

	return hr;
}

