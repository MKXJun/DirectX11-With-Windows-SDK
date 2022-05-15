//***************************************************************************************
// BlurFilter.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 图像模糊滤波器类
// texture blur filter class.
//***************************************************************************************

#ifndef BLURFILTER_H
#define BLURFILTER_H

#include <d3d11_1.h>
#include <wrl/client.h>
#include <string>

class BlurFilter
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    BlurFilter() = default;
    ~BlurFilter() = default;
    // 不允许拷贝，允许移动
    BlurFilter(const BlurFilter&) = delete;
    BlurFilter& operator=(const BlurFilter&) = delete;
    BlurFilter(BlurFilter&&) = default;
    BlurFilter& operator=(BlurFilter&&) = default;

    // 最大模糊半径
    constexpr static int MaxBlurRadius = 9;

    HRESULT InitResource(ID3D11Device* device,
        UINT texWidth,
        UINT texHeight);
    
    // 设置用户自定义权重值与半径(最大为9)
    // 若设置半径为3，则读取weights的前7个数值
    HRESULT SetWeights(float weights[2 * MaxBlurRadius + 1], int blurRadius);

    // 设置高斯滤波(半径最大为9)
    // sigma越大，两边权值越大
    HRESULT SetGaussianWeights(int blurRadius, float sigma);

    // 启动模糊
    void Execute(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* inputTex,				// 输入纹理SRV
        ID3D11UnorderedAccessView* outputTex = nullptr,	// 输出纹理UAV，可为原输入纹理
                                                        // 若为nullptr，后续通过GetOutputTexture获取
        UINT blurTimes = 1);


    // 获取渲染好的纹理的着色器资源视图
    // 仅调用Execute传递的outputTex=nullptr时有意义
    // 引用数不增加，仅用于传参
    ID3D11ShaderResourceView* GetOutputTexture();

    // 设置调试对象名
    void SetDebugObjectName(const std::string& name);

private:
    struct {
        int blurRadius;
        float weights[2 * MaxBlurRadius + 1];
    } m_CBSettings{};

private:

    UINT m_Width = 0;
    UINT m_Height = 0;

    ComPtr<ID3D11Buffer> m_pConstantBuffer;			// 常量缓冲区

    ComPtr<ID3D11ShaderResourceView> m_pTempSRV0;	// 临时缓冲区0的着色器资源视图
    ComPtr<ID3D11ShaderResourceView> m_pTempSRV1;	// 临时缓冲区1的着色器资源视图

    ComPtr<ID3D11UnorderedAccessView> m_pTempUAV0;	// 临时缓冲区0的无序访问视图
    ComPtr<ID3D11UnorderedAccessView> m_pTempUAV1;	// 临时缓冲区1的无序访问视图

    ComPtr<ID3D11ComputeShader> m_pBlurHorzCS;		// 用于水平方向模糊的计算着色器
    ComPtr<ID3D11ComputeShader> m_pBlurVertCS;		// 用于垂直方向模糊的计算着色器
};

#endif