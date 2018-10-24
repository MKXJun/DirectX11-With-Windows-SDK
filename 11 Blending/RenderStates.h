#ifndef RENDERSTATES_H
#define RENDERSTATES_H

#include <wrl/client.h>
#include <d3d11_1.h>


class RenderStates
{
public:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	static bool IsInit();

	static void InitAll(ComPtr<ID3D11Device> device);
	// 使用ComPtr无需手工释放

public:
	static ComPtr<ID3D11RasterizerState> RSWireframe;	// 光栅化器状态：线框模式
	static ComPtr<ID3D11RasterizerState> RSNoCull;		// 光栅化器状态：无背面裁剪模式

	static ComPtr<ID3D11SamplerState> SSLinearWrap;		// 采样器状态：线性过滤
	static ComPtr<ID3D11SamplerState> SSAnistropicWrap;	// 采样器状态：各项异性过滤

	static ComPtr<ID3D11BlendState> BSNoColorWrite;		// 混合状态：不写入颜色
	static ComPtr<ID3D11BlendState> BSTransparent;		// 混合状态：透明混合
	static ComPtr<ID3D11BlendState> BSAlphaToCoverage;	// 混合状态：Alpha-To-Coverage
};



#endif
