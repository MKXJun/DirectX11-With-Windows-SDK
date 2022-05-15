//***************************************************************************************
// SobelFilter.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 图像索贝尔滤波器类
// texture sobel filter class.
//***************************************************************************************

#ifndef SOBELFILTER_H
#define SOBELFILTER_H

#include <d3d11_1.h>
#include <wrl/client.h>
#include <string>

class SobelFilter
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    SobelFilter() = default;
    ~SobelFilter() = default;
    // 不允许拷贝，允许移动
    SobelFilter(const SobelFilter&) = delete;
    SobelFilter& operator=(const SobelFilter&) = delete;
    SobelFilter(SobelFilter&&) = default;
    SobelFilter& operator=(SobelFilter&&) = default;


    HRESULT InitResource(ID3D11Device* device,
        UINT texWidth,
        UINT texHeight);

    // 启动Sobel滤波
    void Execute(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* inputTex,					// 输入纹理SRV
        ID3D11UnorderedAccessView* outputTex = nullptr);	// 输出纹理UAV，不可以为原输入纹理，但可以为nullptr，
                                                            // 后续通过GetOutputTexture获取

    // 获取渲染好的纹理的着色器资源视图
    // 仅调用Execute传递的outputTex=nullptr时有意义
    // 引用数不增加，仅用于传参
    ID3D11ShaderResourceView* GetOutputTexture();

    // 设置调试对象名
    void SetDebugObjectName(const std::string& name);

private:

    UINT m_Width = 0;
    UINT m_Height = 0;

    ComPtr<ID3D11ShaderResourceView> m_pTempSRV;	// 临时缓冲区的着色器资源视图
    ComPtr<ID3D11UnorderedAccessView> m_pTempUAV;	// 临时缓冲区的无序访问视图

    ComPtr<ID3D11ComputeShader> m_pSobelCS;			// 用于Sobel滤波的计算着色器
};

#endif

