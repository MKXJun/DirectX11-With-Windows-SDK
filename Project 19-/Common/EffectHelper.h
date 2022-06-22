//***************************************************************************************
// EffectsHelper.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 定义一些实用的特效助手类
// Define utility effect helper classes.
//*************************************************************************************** 

#pragma once

#ifndef EFFECT_HELPER_H
#define EFFECT_HELPER_H

#include "WinMin.h"
#include <string_view>
#include <memory>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include "Property.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11GeometryShader;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11SamplerState;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11BlendState;
//
// EffectHelper
//

// 渲染通道描述
// 通过指定添加着色器时提供的名字来设置着色器
struct EffectPassDesc
{
    std::string_view nameVS;
    std::string_view nameDS;
    std::string_view nameHS;
    std::string_view nameGS;
    std::string_view namePS;
    std::string_view nameCS;
};

// 常量缓冲区的变量
// 非COM组件
struct IEffectConstantBufferVariable
{
    // 设置无符号整数，也可以为bool设置
    virtual void SetUInt(uint32_t val) = 0;
    // 设置有符号整数
    virtual void SetSInt(int val) = 0;
    // 设置浮点数
    virtual void SetFloat(float val) = 0;

    // 设置无符号整数向量，允许设置1个到4个分量
    // 着色器变量类型为bool也可以使用
    // 根据要设置的分量数来读取data的前几个分量
    virtual void SetUIntVector(uint32_t numComponents, const uint32_t data[4]) = 0;

    // 设置有符号整数向量，允许设置1个到4个分量
    // 根据要设置的分量数来读取data的前几个分量
    virtual void SetSIntVector(uint32_t numComponents, const int data[4]) = 0;

    // 设置浮点数向量，允许设置1个到4个分量
    // 根据要设置的分量数来读取data的前几个分量
    virtual void SetFloatVector(uint32_t numComponents, const float data[4]) = 0;

    // 设置无符号整数矩阵，允许行列数在1-4
    // 要求传入数据没有填充，例如3x3矩阵可以直接传入UINT[3][3]类型
    virtual void SetUIntMatrix(uint32_t rows, uint32_t cols, const uint32_t* noPadData) = 0;

    // 设置有符号整数矩阵，允许行列数在1-4
    // 要求传入数据没有填充，例如3x3矩阵可以直接传入INT[3][3]类型
    virtual void SetSIntMatrix(uint32_t rows, uint32_t cols, const int* noPadData) = 0;

    // 设置浮点数矩阵，允许行列数在1-4
    // 要求传入数据没有填充，例如3x3矩阵可以直接传入FLOAT[3][3]类型
    virtual void SetFloatMatrix(uint32_t rows, uint32_t cols, const float* noPadData) = 0;

    // 设置其余类型，允许指定设置范围
    virtual void SetRaw(const void* data, uint32_t byteOffset = 0, uint32_t byteCount = 0xFFFFFFFF) = 0;

    // 设置属性
    virtual void Set(const Property& prop) = 0;

    // 获取最近一次设置的值，允许指定读取范围
    virtual HRESULT GetRaw(void* pOutput, uint32_t byteOffset = 0, uint32_t byteCount = 0xFFFFFFFF) = 0;

    virtual ~IEffectConstantBufferVariable() {}
};

// 渲染通道
// 非COM组件
class EffectHelper;
struct IEffectPass
{
    // 设置光栅化状态
    virtual void SetRasterizerState(ID3D11RasterizerState* pRS) = 0;
    // 设置混合状态
    virtual void SetBlendState(ID3D11BlendState* pBS, const float blendFactor[4], uint32_t sampleMask) = 0;
    // 设置深度混合状态
    virtual void SetDepthStencilState(ID3D11DepthStencilState* pDSS, uint32_t stencilValue) = 0;

    // 获取顶点着色器的uniform形参用于设置值
    virtual std::shared_ptr<IEffectConstantBufferVariable> VSGetParamByName(std::string_view paramName) = 0;
    // 获取域着色器的uniform形参用于设置值
    virtual std::shared_ptr<IEffectConstantBufferVariable> DSGetParamByName(std::string_view paramName) = 0;
    // 获取外壳着色器的uniform形参用于设置值
    virtual std::shared_ptr<IEffectConstantBufferVariable> HSGetParamByName(std::string_view paramName) = 0;
    // 获取几何着色器的uniform形参用于设置值
    virtual std::shared_ptr<IEffectConstantBufferVariable> GSGetParamByName(std::string_view paramName) = 0;
    // 获取像素着色器的uniform形参用于设置值
    virtual std::shared_ptr<IEffectConstantBufferVariable> PSGetParamByName(std::string_view paramName) = 0;
    // 获取计算着色器的uniform形参用于设置值
    virtual std::shared_ptr<IEffectConstantBufferVariable> CSGetParamByName(std::string_view paramName) = 0;
    // 获取所属特效助理
    virtual EffectHelper* GetEffectHelper() = 0;
    // 获取特效名
    virtual const std::string& GetPassName() = 0;
    
    // 应用着色器、常量缓冲区(包括函数形参)、采样器、着色器资源和可读写资源到渲染管线
    virtual void Apply(ID3D11DeviceContext* deviceContext) = 0;

    // 调度计算着色器
    // 传入线程数目，内部会根据计算着色器的线程组维度调用合适的线程组数目
    virtual void Dispatch(ID3D11DeviceContext* deviceContext, uint32_t threadX = 1, uint32_t threadY = 1, uint32_t threadZ = 1) = 0;

    virtual ~IEffectPass() {};
};

// 特效助理
// 负责管理着色器、采样器、着色器资源、常量缓冲区、着色器形参、可读写资源、渲染状态
class EffectHelper
{
public:

    EffectHelper();
    ~EffectHelper();
    // 不允许拷贝，允许移动
    EffectHelper(const EffectHelper&) = delete;
    EffectHelper& operator=(const EffectHelper&) = delete;
    EffectHelper(EffectHelper&&) = default;
    EffectHelper& operator=(EffectHelper&&) = default;

    // 设置编译好的着色器文件缓存路径并创建
    // 若设置为""，则关闭缓存
    // 若forceWrite为true，每次运行程序都会强制覆盖保存
    // 默认情况下不会缓存编译好的着色器
    // 在shader没完成修改的时候应开启forceWrite
    void SetBinaryCacheDirectory(std::wstring_view cacheDir, bool forceWrite = false);

    // 编译着色器 或 读取着色器字节码，按下述顺序：
    // 1. 如果开启着色器字节码文件缓存路径 且 关闭强制覆盖，则优先尝试读取${cacheDir}/${shaderName}.cso并添加
    // 2. 否则读取filename。若为着色器字节码，直接添加
    // 3. 若filename为hlsl源码，则进行编译和添加。开启着色器字节码文件缓存会保存着色器字节码到${cacheDir}/${shaderName}.cso
    // 注意：
    // 1. 不同着色器代码，若常量缓冲区使用同一个槽，对应的定义应保持完全一致
    // 2. 不同着色器代码，若存在全局变量，定义应保持完全一致
    // 3. 不同着色器代码，若采样器、着色器资源或可读写资源使用同一个槽，对应的定义应保持完全一致，否则只能使用按槽设置
    HRESULT CreateShaderFromFile(std::string_view shaderName, std::wstring_view filename, ID3D11Device* device,
        LPCSTR entryPoint = nullptr, LPCSTR shaderModel = nullptr, const D3D_SHADER_MACRO* pDefines = nullptr, ID3DBlob** ppShaderByteCode = nullptr);

    // 仅编译着色器
    static HRESULT CompileShaderFromFile(std::wstring_view filename, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppShaderByteCode, ID3DBlob** ppErrorBlob = nullptr,
        const D3D_SHADER_MACRO* pDefines = nullptr, ID3DInclude* pInclude = D3D_COMPILE_STANDARD_FILE_INCLUDE);

    // 添加编译好的着色器二进制信息并为其设置标识名
    // 该函数不会保存着色器二进制编码到文件
    // 注意：
    // 1. 不同着色器代码，若常量缓冲区使用同一个register，对应的定义应保持完全一致
    // 2. 不同着色器代码，若存在全局变量，定义应保持完全一致
    // 3. 不同着色器代码，若采样器、着色器资源或可读写资源使用同一个槽，对应的定义应保持完全一致
    HRESULT AddShader(std::string_view name, ID3D11Device* device, ID3DBlob* blob);

    // 添加带流输出的几何着色器并为其设置标识名
    // 该函数不会保存着色器二进制编码到文件
    // 注意：
    // 1. 不同着色器代码，若常量缓冲区使用同一个槽，对应的定义应保持完全一致
    // 2. 不同着色器代码，若存在全局变量，定义应保持完全一致
    // 3. 不同着色器代码，若采样器、着色器资源或可读写资源使用同一个槽，对应的定义应保持完全一致，否则只能使用按槽设置 
    HRESULT AddGeometryShaderWithStreamOutput(std::string_view name, ID3D11Device* device, ID3D11GeometryShader* gsWithSO, ID3DBlob* blob);

    // 清空所有内容
    void Clear();

    // 创建渲染通道
    HRESULT AddEffectPass(std::string_view effectPassName, ID3D11Device* device, const EffectPassDesc* pDesc);
    // 获取特定渲染通道
    std::shared_ptr<IEffectPass> GetEffectPass(std::string_view effectPassName);

    // 获取常量缓冲区的变量用于设置值
    std::shared_ptr<IEffectConstantBufferVariable> GetConstantBufferVariable(std::string_view name);

    // 按槽设置采样器状态
    void SetSamplerStateBySlot(uint32_t slot, ID3D11SamplerState* samplerState);
    // 按名设置采样器状态(若存在同槽多名称则只能使用按槽设置)
    void SetSamplerStateByName(std::string_view name, ID3D11SamplerState* samplerState);
    // 按名映射采样器状态槽(找不到返回-1)
    int MapSamplerStateSlot(std::string_view name);

    // 按槽设置着色器资源
    void SetShaderResourceBySlot(uint32_t slot, ID3D11ShaderResourceView* srv);
    // 按名设置着色器资源(若存在同槽多名称则只能使用按槽设置)
    void SetShaderResourceByName(std::string_view name, ID3D11ShaderResourceView* srv);
    // 按名映射着色器资源槽(找不到返回-1)
    int MapShaderResourceSlot(std::string_view name);

    // 按槽设置可读写资源
    void SetUnorderedAccessBySlot(uint32_t slot, ID3D11UnorderedAccessView* uav, uint32_t* pInitialCount = nullptr);
    // 按名设置可读写资源(若存在同槽多名称则只能使用按槽设置)
    void SetUnorderedAccessByName(std::string_view name, ID3D11UnorderedAccessView* uav, uint32_t* pInitialCount = nullptr);
    // 按名映射可读写资源槽(找不到返回-1)
    int MapUnorderedAccessSlot(std::string_view name);


    // 设置调试对象名
    void SetDebugObjectName(std::string name);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};



#endif
