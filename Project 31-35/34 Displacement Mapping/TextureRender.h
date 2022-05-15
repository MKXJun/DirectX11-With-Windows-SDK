//***************************************************************************************
// TextureRender.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 渲染到纹理类
// Render-To-Texture class.
//***************************************************************************************

#ifndef TEXTURERENDER_H
#define TEXTURERENDER_H

#include <d3d11_1.h>
#include <wrl/client.h>
#include <string>

class TextureRender
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    TextureRender() = default;
    ~TextureRender() = default;
    // 不允许拷贝，允许移动
    TextureRender(const TextureRender&) = delete;
    TextureRender& operator=(const TextureRender&) = delete;
    TextureRender(TextureRender&&) = default;
    TextureRender& operator=(TextureRender&&) = default;


    HRESULT InitResource(ID3D11Device* device,
        int texWidth,
        int texHeight,
        bool shadowMap = false,
        bool generateMips = false);

    // 开始对当前纹理进行渲染
    // 阴影贴图无需提供背景色
    void Begin(ID3D11DeviceContext* deviceContext, const FLOAT backgroundColor[4]);
    // 结束对当前纹理的渲染，还原状态
    void End(ID3D11DeviceContext * deviceContext);
    // 获取渲染好的纹理的着色器资源视图
    // 阴影贴图返回的是深度缓冲区
    // 引用数不增加，仅用于传参
    ID3D11ShaderResourceView* GetOutputTexture();

    // 设置调试对象名
    void SetDebugObjectName(const std::string& name);

private:
    ComPtr<ID3D11ShaderResourceView>	m_pOutputTextureSRV;	// 输出的纹理(或阴影贴图)对应的着色器资源视图
    ComPtr<ID3D11RenderTargetView>		m_pOutputTextureRTV;	// 输出的纹理对应的渲染目标视图
    ComPtr<ID3D11DepthStencilView>		m_pOutputTextureDSV;	// 输出纹理所用的深度/模板视图(或阴影贴图)
    D3D11_VIEWPORT						m_OutputViewPort = {};	// 输出所用的视口

    ComPtr<ID3D11RenderTargetView>		m_pCacheRTV;		    // 临时缓存的后备缓冲区
    ComPtr<ID3D11DepthStencilView>		m_pCacheDSV;		    // 临时缓存的深度/模板缓冲区
    D3D11_VIEWPORT						m_CacheViewPort = {};	// 临时缓存的视口

    bool								m_GenerateMips = false;	// 是否生成mipmap链
    bool								m_ShadowMap = false;	// 是否为阴影贴图

};

#endif
