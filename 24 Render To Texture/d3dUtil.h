#ifndef D3DUTIL_H
#define D3DUTIL_H

#include <d3d11_1.h>			// 已包含Windows.h
#include <DirectXCollision.h>	// 已包含DirectXMath.h
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <vector>
#include <string>
#include <filesystem>
#include "dxerr.h"
#include "ScreenGrab.h"
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

HRESULT CreateShaderFromFile(const WCHAR * objFileNameInOut, const WCHAR * hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob ** ppBlobOut);


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



//
// 纹理立方体相关函数
//

// 根据给定的一张包含立方体六个面的纹理，创建纹理立方体
// 要求纹理宽高比为4:3，且按下面形式布局:
// .  +Y .  .
// -X +Z +X -Z 
// .  -Y .  .
// 该函数默认不生成mipmap(即等级仅为1)，若需要则设置generateMips为true
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateWICTextureCubeFromFile(
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext,
	std::wstring cubemapFileName,
	bool generateMips = false);

// 根据按D3D11_TEXTURECUBE_FACE索引顺序给定的六张纹理，创建纹理立方体
// 要求纹理是同样大小的正方形
// 该函数默认不生成mipmap(即等级仅为1)，若需要则设置generateMips为true
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateWICTextureCubeFromFile(
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext,
	std::vector<std::wstring> cubemapFileNames,
	bool generateMips = false);

#endif
