//***************************************************************************************
// OITRender.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 顺序无关透明度绘制类
// OIT render class.
//***************************************************************************************

#ifndef OITRENDER_H
#define OITRENDER_H

#include <d3d11_1.h>
#include <wrl/client.h>
#include <string>

class OITRender
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    OITRender() = default;
    ~OITRender() = default;
    // 不允许拷贝，允许移动
    OITRender(const OITRender&) = delete;
    OITRender& operator=(const OITRender&) = delete;
    OITRender(OITRender&&) = default;
    OITRender& operator=(OITRender&&) = default;

    HRESULT InitResource(ID3D11Device* device, 
        UINT width,			// 帧宽度
        UINT height,		// 帧高度
        UINT multiple);		// 用多少倍于帧像素数的缓冲区存储像素片元

    // 开始收集透明物体像素片元
    void BeginDefaultStore(ID3D11DeviceContext* deviceContext);
    // 结束收集，还原状态
    void EndStore(ID3D11DeviceContext* deviceContext);
    
    // 将背景与透明物体像素片元混合完成最终渲染
    void Draw(ID3D11DeviceContext * deviceContext, ID3D11ShaderResourceView* background);

    void SetDebugObjectName(const std::string& name);

private:
    struct {
        int width;
        int height;
        int pad1;
        int pad2;
    } m_CBFrame{};												// 对应OIT.hlsli的常量缓冲区
private:
    ComPtr<ID3D11InputLayout> m_pInputLayout;					// 绘制屏幕的顶点输入布局

    ComPtr<ID3D11Buffer> m_pFLBuffer;							// 片元/链接缓冲区
    ComPtr<ID3D11Buffer> m_pStartOffsetBuffer;					// 起始偏移缓冲区
    ComPtr<ID3D11Buffer> m_pVertexBuffer;						// 绘制背景用的顶点缓冲区
    ComPtr<ID3D11Buffer> m_pIndexBuffer;						// 绘制背景用的索引缓冲区
    ComPtr<ID3D11Buffer> m_pConstantBuffer;						// 常量缓冲区

    ComPtr<ID3D11ShaderResourceView> m_pFLBufferSRV;			// 片元/链接缓冲区的着色器资源视图
    ComPtr<ID3D11ShaderResourceView> m_pStartOffsetBufferSRV;	// 起始偏移缓冲区的着色器资源视图

    ComPtr<ID3D11UnorderedAccessView> m_pFLBufferUAV;			// 片元/链接缓冲区的无序访问视图
    ComPtr<ID3D11UnorderedAccessView> m_pStartOffsetBufferUAV;	// 起始偏移缓冲区的无序访问视图

    ComPtr<ID3D11VertexShader> m_pOITRenderVS;					// 透明混合渲染的顶点着色器
    ComPtr<ID3D11PixelShader> m_pOITRenderPS;					// 透明混合渲染的像素着色器
    ComPtr<ID3D11PixelShader> m_pOITStorePS;					// 用于存储透明像素片元的像素着色器
    
    ComPtr<ID3D11PixelShader> m_pCachePS;						// 临时缓存的像素着色器

    UINT m_FrameWidth = 0;										// 帧像素宽度
    UINT m_FrameHeight = 0;										// 帧像素高度
    UINT m_IndexCount = 0;										// 绘制索引数
};


#endif
