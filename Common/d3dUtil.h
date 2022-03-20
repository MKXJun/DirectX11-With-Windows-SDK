//***************************************************************************************
// d3dUtil.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// D3D实用工具集
// Direct3D utility tools.
//***************************************************************************************

#pragma once

#ifndef D3DUTIL_H
#define D3DUTIL_H

#include <d3d11_1.h>			// 已包含Windows.h
#include <DirectXMath.h>        // 已包含DirectXMath.h
#include <d3dcompiler.h>
#include <vector>
#include <string>
#include <string_view>

//
// 宏相关
//

// 默认开启图形调试器具名化
// 如果不需要该项功能，可通过全局文本替换将其值设置为0
#ifndef GRAPHICS_DEBUGGER_OBJECT_NAME
#define GRAPHICS_DEBUGGER_OBJECT_NAME (1)
#endif

// 安全COM组件释放宏
#define SAFE_RELEASE(p) { if ((p)) { (p)->Release(); (p) = nullptr; } }

//
// 文本转换函数
//

std::wstring UTF8ToWString(std::string_view utf8str);

std::string WStringToUTF8(std::wstring_view wstr);

//
// 获取ID
//
size_t StringToID(std::string_view str);

//
// 辅助调试相关函数
//

// ------------------------------
// D3D11SetDebugObjectName函数
// ------------------------------
// 为D3D设备创建出来的对象在图形调试器中设置对象名
// [In]resource				D3D11设备创建出的对象
// [In]name					对象名
inline void D3D11SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_ std::string_view name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.data());
#else
	UNREFERENCED_PARAMETER(resource);
	UNREFERENCED_PARAMETER(name);
#endif
}

// ------------------------------
// D3D11SetDebugObjectName函数
// ------------------------------
// 为D3D设备创建出来的对象在图形调试器中清空对象名
// [In]resource				D3D11设备创建出的对象
inline void D3D11SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_ std::nullptr_t)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
#else
	UNREFERENCED_PARAMETER(resource);
#endif
}



// ------------------------------
// DXGISetDebugObjectName函数
// ------------------------------
// 为DXGI对象在图形调试器中设置对象名
// [In]object				DXGI对象
// [In]name					对象名
inline void DXGISetDebugObjectName(_In_ IDXGIObject* object, _In_ std::string_view name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.data());
#else
	UNREFERENCED_PARAMETER(object);
	UNREFERENCED_PARAMETER(name);
#endif
}

// ------------------------------
// DXGISetDebugObjectName函数
// ------------------------------
// 为DXGI对象在图形调试器中清空对象名
// [In]object				DXGI对象
inline void DXGISetDebugObjectName(_In_ IDXGIObject* object, _In_ std::nullptr_t)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
#else
	UNREFERENCED_PARAMETER(object);
#endif
}

//
// 着色器编译相关函数
//

// ------------------------------
// CreateShaderFromFile函数
// ------------------------------
// [In]csoFileNameInOut 编译好的着色器二进制文件(.cso)，若有指定则优先寻找该文件并读取
// [In]hlslFileName     着色器代码，若未找到着色器二进制文件则编译着色器代码
// [In]pDefines         预添加宏定义数组，以{nullptr, nullptr}元素结尾
// [In]entryPoint       入口点(指定开始的函数)
// [In]shaderModel      着色器模型，格式为"*s_5_0"，*可以为c,d,g,h,p,v之一
// [Out]ppBlobOut       输出着色器二进制信息
HRESULT CreateShaderFromFile(
	const WCHAR* csoFileNameInOut,
	const WCHAR* hlslFileName,
	const D3D_SHADER_MACRO* pDefines,
	LPCSTR entryPoint,
	LPCSTR shaderModel,
	ID3DBlob** ppBlobOut);


//
// 数学相关函数
//

namespace XMath
{
	// ------------------------------
	// InverseTranspose函数
	// ------------------------------
	inline DirectX::XMMATRIX XM_CALLCONV InverseTranspose(DirectX::FXMMATRIX M)
	{
		using namespace DirectX;

		// 世界矩阵的逆的转置仅针对法向量，我们也不需要世界矩阵的平移分量
		// 而且不去掉的话，后续再乘上观察矩阵之类的就会产生错误的变换结果
		XMMATRIX A = M;
		A.r[3] = g_XMIdentityR3;

		return XMMatrixTranspose(XMMatrixInverse(nullptr, A));
	}

	inline float Lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}

	inline float Clamp(float val, float minVal, float maxVal)
	{
		return (std::min)((std::max)(val, minVal), maxVal);
	}
}


#endif
