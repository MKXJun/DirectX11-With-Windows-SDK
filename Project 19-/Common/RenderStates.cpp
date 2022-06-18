#include "XUtil.h"
#include "RenderStates.h"
#include "DXTrace.h"

using namespace Microsoft::WRL;

ComPtr<ID3D11RasterizerState> RenderStates::RSNoCull			= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSWireframe			= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSCullClockWise		= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSShadow		    = nullptr;

ComPtr<ID3D11SamplerState> RenderStates::SSPointClamp			= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSLinearWrap			= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSLinearClamp          = nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSAnistropicWrap16x    = nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSAnistropicClamp2x    = nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSAnistropicClamp4x    = nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSAnistropicClamp8x    = nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSAnistropicClamp16x   = nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSShadowPCF			= nullptr;

ComPtr<ID3D11BlendState> RenderStates::BSAlphaToCoverage		= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSTransparent			= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSAdditive				= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSAlphaWeightedAdditive  = nullptr;

ComPtr<ID3D11DepthStencilState> RenderStates::DSSEqual          = nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSLessEqual      = nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSGreaterEqual   = nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSNoDepthWrite   = nullptr;
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
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = false;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 1.0f;
    HR(device->CreateRasterizerState(&rasterizerDesc, RSShadow.GetAddressOf()));

    // ******************
    // 初始化采样器状态
    //
    CD3D11_SAMPLER_DESC sampDesc(CD3D11_DEFAULT{});

    // 点过滤与Clamp模式
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    HR(device->CreateSamplerState(&sampDesc, SSPointClamp.GetAddressOf()));

    // 线性过滤与Clamp模式
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    HR(device->CreateSamplerState(&sampDesc, SSLinearClamp.GetAddressOf()));

    // 2倍各向异性过滤与Clamp模式
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.MaxAnisotropy = 2;
    HR(device->CreateSamplerState(&sampDesc, SSAnistropicClamp2x.GetAddressOf()));

    // 4倍各向异性过滤与Clamp模式
    sampDesc.MaxAnisotropy = 4;
    HR(device->CreateSamplerState(&sampDesc, SSAnistropicClamp4x.GetAddressOf()));

    // 8倍各向异性过滤与Clamp模式
    sampDesc.MaxAnisotropy = 8;
    HR(device->CreateSamplerState(&sampDesc, SSAnistropicClamp8x.GetAddressOf()));

    // 16倍各向异性过滤与Clamp模式
    sampDesc.MaxAnisotropy = 16;
    HR(device->CreateSamplerState(&sampDesc, SSAnistropicClamp16x.GetAddressOf()));

    // 线性过滤与Wrap模式
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.MaxAnisotropy = 0;
    HR(device->CreateSamplerState(&sampDesc, SSLinearWrap.GetAddressOf()));

    // 16倍各向异性过滤与Wrap模式
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.MaxAnisotropy = 16;
    HR(device->CreateSamplerState(&sampDesc, SSAnistropicWrap16x.GetAddressOf()));

    // 采样器状态：深度比较与Border模式
    sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
    sampDesc.MaxAnisotropy = 1;
    sampDesc.MinLOD = 0.0f;
    sampDesc.MaxLOD = 0.0f;
    sampDesc.BorderColor[0] = 0.0f;
    sampDesc.BorderColor[1] = 0.0f;
    sampDesc.BorderColor[2] = 0.0f;
    sampDesc.BorderColor[3] = 1.0f;
    HR(device->CreateSamplerState(&sampDesc, SSShadowPCF.GetAddressOf()));

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

    // 带Alpha权重的加法混合模式
    // Color = SrcAlpha * SrcColor + DestColor
    // Alpha = SrcAlpha
    rtDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rtDesc.DestBlend = D3D11_BLEND_ONE;
    rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
    rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
    rtDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
    rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    HR(device->CreateBlendState(&blendDesc, BSAlphaWeightedAdditive.GetAddressOf()));

    // ******************
    // 初始化深度/模板状态
    //
    CD3D11_DEPTH_STENCIL_DESC dsDesc(CD3D11_DEFAULT{});
    // 仅允许深度值一致的像素进行写入的深度/模板状态
    // 没必要写入深度
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
    HR(device->CreateDepthStencilState(&dsDesc, DSSEqual.GetAddressOf()));

    // LESS_EQUAL测试
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    HR(device->CreateDepthStencilState(&dsDesc, DSSLessEqual.GetAddressOf()));

    // 反向Z => GREATER_EQUAL测试
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    HR(device->CreateDepthStencilState(&dsDesc, DSSGreaterEqual.GetAddressOf()));

    // 进行深度测试，但不写入深度值的状态
    // 若绘制非透明物体时，应使用默认状态
    // 绘制透明物体时，使用该状态可以有效确保混合状态的进行
    // 并且确保较前的非透明物体可以阻挡较后的一切物体
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = false;
    HR(device->CreateDepthStencilState(&dsDesc, DSSNoDepthWrite.GetAddressOf()));

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
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(RSCullClockWise.Get(), "RSCullClockWise");
    SetDebugObjectName(RSNoCull.Get(), "RSNoCull");
    SetDebugObjectName(RSWireframe.Get(), "RSWireframe");
    SetDebugObjectName(RSShadow.Get(), "RSShadow");

    SetDebugObjectName(SSPointClamp.Get(), "SSPointClamp");
    SetDebugObjectName(SSLinearWrap.Get(), "SSLinearWrap");
    SetDebugObjectName(SSLinearClamp.Get(), "SSLinearClamp");
    SetDebugObjectName(SSAnistropicWrap16x.Get(), "SSAnistropicWrap16x");
    SetDebugObjectName(SSAnistropicClamp2x.Get(), "SSAnistropicClamp2x");
    SetDebugObjectName(SSAnistropicClamp4x.Get(), "SSAnistropicClamp4x");
    SetDebugObjectName(SSAnistropicClamp8x.Get(), "SSAnistropicClamp8x");
    SetDebugObjectName(SSAnistropicClamp16x.Get(), "SSAnistropicClamp16x");
    SetDebugObjectName(SSShadowPCF.Get(), "SSShadowPCF");

    SetDebugObjectName(BSAlphaToCoverage.Get(), "BSAlphaToCoverage");
    SetDebugObjectName(BSTransparent.Get(), "BSTransparent");
    SetDebugObjectName(BSAdditive.Get(), "BSAdditive");

    SetDebugObjectName(DSSEqual.Get(), "DSSEqual");
    SetDebugObjectName(DSSGreaterEqual.Get(), "DSSGreaterEqual");
    SetDebugObjectName(DSSLessEqual.Get(), "DSSLessEqual");
    SetDebugObjectName(DSSNoDepthWrite.Get(), "DSSNoDepthWrite");
    SetDebugObjectName(DSSNoDepthTest.Get(), "DSSNoDepthTest");
    SetDebugObjectName(DSSWriteStencil.Get(), "DSSWriteStencil");
    SetDebugObjectName(DSSEqualStencil.Get(), "DSSEqualStencil");
#endif
}