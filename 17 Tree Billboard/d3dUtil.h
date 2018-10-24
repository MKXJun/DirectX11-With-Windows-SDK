#ifndef D3DUTIL_H
#define D3DUTIL_H

#include <d3d11_1.h>			// 已包含Windows.h
#include <DirectXCollision.h>	// 已包含DirectXMath.h
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <DDSTextureLoader.h>	
#include <WICTextureLoader.h>
#include <wrl/client.h>
#include <filesystem>
#include <vector>
#include <string>
#include "dxerr.h"

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

// objFileNameInOut为编译好的着色器二进制文件(.*so)，若有指定则优先寻找该文件并读取
// hlslFileName为着色器代码，若未找到着色器二进制文件则编译着色器代码
// 编译成功后，若指定了objFileNameInOut，则保存编译好的着色器二进制信息到该文件
// ppBlobOut输出着色器二进制信息
HRESULT CreateShaderFromFile(const WCHAR* objFileNameInOut, const WCHAR* hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppBlobOut);

//
// 纹理数组相关函数
//

// 根据给定的DDS纹理文件集合，创建2D纹理数组
// 要求所有纹理的宽度和高度都一致
// 若maxMipMapSize为0，使用默认mipmap等级
// 否则，mipmap等级将不会超过maxMipMapSize
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateDDSTexture2DArrayFromFile(
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext,
	const std::vector<std::wstring>& filenames,
	UINT maxMipMapSize = 0);





#endif
