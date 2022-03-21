#include "XUtil.h"
#include "RenderStates.h"
#include "DXTrace.h"

using namespace Microsoft::WRL;

ComPtr<ID3D11RasterizerState> RenderStates::RSNoCull			= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSWireframe			= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSCullClockWise		= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSDepthBias		    = nullptr;

ComPtr<ID3D11SamplerState> RenderStates::SSPointClamp			= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSAnistropicWrap16x	= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSLinearWrap			= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSShadow				= nullptr;

ComPtr<ID3D11BlendState> RenderStates::BSAlphaToCoverage		= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSTransparent			= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSAdditive				= nullptr;

ComPtr<ID3D11DepthStencilState> RenderStates::DSSEqual          = nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSGreaterEqual   = nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSNoDepthTest    = nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSWriteStencil	= nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSEqualStencil   = nullptr;


bool RenderStates::IsInit()
{
	// 一般来说初始化操作会把所有的状态都创建出来
	return RSWireframe != nullptr;
}

void RenderStates::InitAll(ID3D11Device* device)
{
	// 先前初始化过的话就没必要重来了
	if (IsInit())
		return;
	// ******************
	// 初始化光栅化器状态
	//
	CD3D11_RASTERIZER_DESC rasterizerDesc(CD3D11_DEFAULT{});

	// 线框模式
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	HR(device->CreateRasterizerState(&rasterizerDesc, RSWireframe.GetAddressOf()));

	// 无背面剔除模式
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = false;
	HR(device->CreateRasterizerState(&rasterizerDesc, RSNoCull.GetAddressOf()));

	// 顺时针剔除模式
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = true;
	HR(device->CreateRasterizerState(&rasterizerDesc, RSCullClockWise.GetAddressOf()));

	// 深度偏移模式
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthBias = 100000;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 1.0f;
	HR(device->CreateRasterizerState(&rasterizerDesc, RSDepthBias.GetAddressOf()));

	// ******************
	// 初始化采样器状态
	//
	CD3D11_SAMPLER_DESC sampDesc(CD3D11_DEFAULT{});

	// 点过滤与Clamp模式
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	HR(device->CreateSamplerState(&sampDesc, SSPointClamp.GetAddressOf()));

	// 线性过滤与Wrap模式
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	HR(device->CreateSamplerState(&sampDesc, SSLinearWrap.GetAddressOf()));

	// 16倍各向异性过滤与Wrap模式
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.MaxAnisotropy = 16;
	HR(device->CreateSamplerState(&sampDesc, SSAnistropicWrap16x.GetAddressOf()));

	// 采样器状态：深度比较与Border模式
	// 注意：反向Z
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_GREATER_EQUAL;
	sampDesc.BorderColor[0] = 1.0f;
	sampDesc.BorderColor[1] = 0.0f;
	sampDesc.BorderColor[2] = 0.0f;
	sampDesc.BorderColor[3] = 0.0f;
	HR(device->CreateSamplerState(&sampDesc, SSShadow.GetAddressOf()));

	// ******************
	// 初始化混合状态
	//
	CD3D11_BLEND_DESC blendDesc(CD3D11_DEFAULT{});
	auto& rtDesc = blendDesc.RenderTarget[0];

	// Alpha-To-Coverage模式
	blendDesc.AlphaToCoverageEnable = true;
	HR(device->CreateBlendState(&blendDesc, BSAlphaToCoverage.GetAddressOf()));

	// 透明混合模式
	// Color = SrcAlpha * SrcColor + (1 - SrcAlpha) * DestColor 
	// Alpha = SrcAlpha
	blendDesc.AlphaToCoverageEnable = false;
	rtDesc.BlendEnable = true;
	rtDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	HR(device->CreateBlendState(&blendDesc, BSTransparent.GetAddressOf()));

	// 加法混合模式
	// Color = SrcColor + DestColor
	// Alpha = SrcAlpha + DestAlpha
	rtDesc.SrcBlend = D3D11_BLEND_ONE;
	rtDesc.DestBlend = D3D11_BLEND_ONE;
	rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.DestBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	HR(device->CreateBlendState(&blendDesc, BSAdditive.GetAddressOf()));

	// ******************
	// 初始化深度/模板状态
	//
	CD3D11_DEPTH_STENCIL_DESC dsDesc(CD3D11_DEFAULT{});
	// 仅允许深度值一致的像素进行写入的深度/模板状态
	// 没必要写入深度
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
	HR(device->CreateDepthStencilState(&dsDesc, DSSEqual.GetAddressOf()));

	// 反向Z => GREATER_EQUAL测试
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
	HR(device->CreateDepthStencilState(&dsDesc, DSSGreaterEqual.GetAddressOf()));

	// 关闭深度测试的深度/模板状态
	// 若绘制非透明物体，务必严格按照绘制顺序
	// 绘制透明物体则不需要担心绘制顺序
	// 而默认情况下模板测试就是关闭的
	dsDesc.DepthEnable = false;
	HR(device->CreateDepthStencilState(&dsDesc, DSSNoDepthTest.GetAddressOf()));

	// 反向Z深度测试，模板值比较
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
	dsDesc.StencilEnable = true;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	HR(device->CreateDepthStencilState(&dsDesc, DSSEqualStencil.GetAddressOf()));

	// 无深度测试，仅模板写入
	dsDesc.DepthEnable = false;
	dsDesc.StencilEnable = true;
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	HR(device->CreateDepthStencilState(&dsDesc, DSSWriteStencil.GetAddressOf()));


	// ******************
	// 设置调试对象名
	//
	D3D11SetDebugObjectName(RSCullClockWise.Get(), "RSCullClockWise");
	D3D11SetDebugObjectName(RSNoCull.Get(), "RSNoCull");
	D3D11SetDebugObjectName(RSWireframe.Get(), "RSWireframe");
	D3D11SetDebugObjectName(RSDepthBias.Get(), "RSDepthBias");

	D3D11SetDebugObjectName(SSLinearWrap.Get(), "SSLinearWrap");
	D3D11SetDebugObjectName(SSAnistropicWrap16x.Get(), "SSAnistropicWrap16x");
	D3D11SetDebugObjectName(SSShadow.Get(), "SSShadow");

	D3D11SetDebugObjectName(BSAlphaToCoverage.Get(), "BSAlphaToCoverage");
	D3D11SetDebugObjectName(BSTransparent.Get(), "BSTransparent");
	D3D11SetDebugObjectName(BSAdditive.Get(), "BSAdditive");

	D3D11SetDebugObjectName(DSSEqual.Get(), "DSSEqual");
	D3D11SetDebugObjectName(DSSGreaterEqual.Get(), "DSSGreaterEqual");
	D3D11SetDebugObjectName(DSSNoDepthTest.Get(), "DSSNoDepthTest");
	D3D11SetDebugObjectName(DSSWriteStencil.Get(), "DSSWriteStencil");
	D3D11SetDebugObjectName(DSSEqualStencil.Get(), "DSSEqualStencil");
}