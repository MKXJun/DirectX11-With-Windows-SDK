//***************************************************************************************
// RenderStates.h by X_Jun(MKXJun) (C) 2018-2019 All Rights Reserved.
// Licensed under the MIT License.
//
// 提供一些渲染状态
// Provide some render states.
//***************************************************************************************

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

	static void InitAll(ID3D11Device * device);
	// 使用ComPtr无需手工释放

public:
	static ComPtr<ID3D11RasterizerState> RSWireframe;		            // 光栅化器状态：线框模式
	static ComPtr<ID3D11RasterizerState> RSNoCull;			            // 光栅化器状态：无背面裁剪模式
	static ComPtr<ID3D11RasterizerState> RSCullClockWise;	            // 光栅化器状态：顺时针裁剪模式

	static ComPtr<ID3D11SamplerState> SSLinearWrap;			            // 采样器状态：线性过滤
	static ComPtr<ID3D11SamplerState> SSAnistropicWrap;		            // 采样器状态：各项异性过滤

	static ComPtr<ID3D11BlendState> BSNoColorWrite;		                // 混合状态：不写入颜色
	static ComPtr<ID3D11BlendState> BSTransparent;		                // 混合状态：透明混合
	static ComPtr<ID3D11BlendState> BSAlphaToCoverage;	                // 混合状态：Alpha-To-Coverage
	static ComPtr<ID3D11BlendState> BSAdditive;			                // 混合状态：加法混合


	static ComPtr<ID3D11DepthStencilState> DSSWriteStencil;		        // 深度/模板状态：写入模板值
	static ComPtr<ID3D11DepthStencilState> DSSDrawWithStencil;	        // 深度/模板状态：对指定模板值的区域进行绘制
	static ComPtr<ID3D11DepthStencilState> DSSNoDoubleBlend;	        // 深度/模板状态：无二次混合区域
	static ComPtr<ID3D11DepthStencilState> DSSNoDepthTest;		        // 深度/模板状态：关闭深度测试
	static ComPtr<ID3D11DepthStencilState> DSSNoDepthWrite;		        // 深度/模板状态：仅深度测试，不写入深度值
	static ComPtr<ID3D11DepthStencilState> DSSNoDepthTestWithStencil;	// 深度/模板状态：关闭深度测试，对指定模板值的区域进行绘制
	static ComPtr<ID3D11DepthStencilState> DSSNoDepthWriteWithStencil;	// 深度/模板状态：仅深度测试，不写入深度值，对指定模板值的区域进行绘制
};



#endif
