#ifndef D3DUTIL_H
#define D3DUTIL_H

#include <d3d11_1.h>			// 已包含Windows.h
#include <DirectXCollision.h>	// 已包含DirectXMath.h
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <filesystem>
#include <vector>
#include <string>
#include "dxerr.h"
#include "DDSTextureLoader.h"	
#include "WICTextureLoader.h"

// 移植过来的错误检查，该项目仅允许使用Unicode字符集
#if defined(DEBUG) | defined(_DEBUG)
#ifndef HR
#define HR(x)                                              \
{                                                          \
	HRESULT hr = (x);                                      \
	if(FAILED(hr))                                         \
	{                                                      \
		DXTrace(__FILEW__, (DWORD)__LINE__, hr, L#x, true);\
	}                                                      \
}
#endif
#else
#ifndef HR
#define HR(x) (x)
#endif 
#endif

//
// 着色器编译相关函数
//

// ------------------------------
// CreateShaderFromFile函数
// ------------------------------
// [In]objFileNameInOut 编译好的着色器二进制文件(.cso)，若有指定则优先寻找该文件并读取
// [In]hlslFileName     着色器代码，若未找到着色器二进制文件则编译着色器代码
// [In]entryPoint       入口点(指定开始的函数)
// [In]shaderModel      着色器模型，格式为"*s_5_0"，*可以为c,d,g,h,p,v之一
// [Out]ppBlobOut       输出着色器二进制信息
HRESULT CreateShaderFromFile(const WCHAR* objFileNameInOut, const WCHAR* hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppBlobOut);





#endif
