//***************************************************************************************
// XUtil.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 实用工具集
// utility tools.
//***************************************************************************************

#pragma once

#ifndef XUTIL_H
#define XUTIL_H

#include "WinMin.h"
#include <d3d11_1.h>
#include <DirectXMath.h>
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

inline std::wstring UTF8ToWString(std::string_view utf8str)
{
	if (utf8str.empty()) return std::wstring();
	int cbMultiByte = static_cast<int>(utf8str.size());
	int req = MultiByteToWideChar(CP_UTF8, 0, utf8str.data(), cbMultiByte, nullptr, 0);
	std::wstring res(req, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8str.data(), cbMultiByte, &res[0], req);
	return res;
}

inline std::string WStringToUTF8(std::wstring_view wstr)
{
	if (wstr.empty()) return std::string();
	int cbMultiByte = static_cast<int>(wstr.size());
	int req = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), cbMultiByte, nullptr, 0, nullptr, nullptr);
	std::string res(req, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), cbMultiByte, &res[0], req, nullptr, nullptr);
	return res;
}

//
// 获取ID
//
inline size_t StringToID(std::string_view str)
{
	static std::hash<std::string_view> hash;
	return hash(str);
}

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
		return (1.0f - t) * a + t * b;
	}

	inline float Clamp(float val, float minVal, float maxVal)
	{
		return (std::min)((std::max)(val, minVal), maxVal);
	}
}


#endif
