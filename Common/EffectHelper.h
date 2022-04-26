//***************************************************************************************
// EffectsHelper.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 定义一些实用的特效助手类
// Define utility effect helper classes.
//*************************************************************************************** 

#pragma once

#ifndef EFFECTHELPER_H
#define EFFECTHELPER_H

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
	virtual void SetUInt(UINT val) = 0;
	// 设置有符号整数
	virtual void SetSInt(INT val) = 0;
	// 设置浮点数
	virtual void SetFloat(FLOAT val) = 0;

	// 设置无符号整数向量，允许设置1个到4个分量
	// 着色器变量类型为bool也可以使用
	// 根据要设置的分量数来读取data的前几个分量
	virtual void SetUIntVector(UINT numComponents, const UINT data[4]) = 0;

	// 设置有符号整数向量，允许设置1个到4个分量
	// 根据要设置的分量数来读取data的前几个分量
	virtual void SetSIntVector(UINT numComponents, const INT data[4]) = 0;

	// 设置浮点数向量，允许设置1个到4个分量
	// 根据要设置的分量数来读取data的前几个分量
	virtual void SetFloatVector(UINT numComponents, const FLOAT data[4]) = 0;

	// 设置无符号整数矩阵，允许行列数在1-4
	// 要求传入数据没有填充，例如3x3矩阵可以直接传入UINT[3][3]类型
	virtual void SetUIntMatrix(UINT rows, UINT cols, const UINT* noPadData) = 0;

	// 设置有符号整数矩阵，允许行列数在1-4
	// 要求传入数据没有填充，例如3x3矩阵可以直接传入INT[3][3]类型
	virtual void SetSIntMatrix(UINT rows, UINT cols, const INT* noPadData) = 0;

	// 设置浮点数矩阵，允许行列数在1-4
	// 要求传入数据没有填充，例如3x3矩阵可以直接传入FLOAT[3][3]类型
	virtual void SetFloatMatrix(UINT rows, UINT cols, const FLOAT* noPadData) = 0;

	// 设置其余类型，允许指定设置范围
	virtual void SetRaw(const void* data, UINT byteOffset = 0, UINT byteCount = 0xFFFFFFFF) = 0;

	// 设置属性
	virtual void Set(const Property& prop) = 0;

	// 获取最近一次设置的值，允许指定读取范围
	virtual HRESULT GetRaw(void* pOutput, UINT byteOffset = 0, UINT byteCount = 0xFFFFFFFF) = 0;

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
	virtual void SetBlendState(ID3D11BlendState* pBS, const FLOAT blendFactor[4], UINT sampleMask) = 0;
	// 设置深度混合状态
	virtual void SetDepthStencilState(ID3D11DepthStencilState* pDSS, UINT stencilValue) = 0;

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

	// 添加着色器并为其设置标识名
	// 注意：
	// 1. 不同着色器代码，若常量缓冲区使用同一个槽，对应的定义应保持完全一致
	// 2. 不同着色器代码，若存在全局变量，定义应保持完全一致
	// 3. 不同着色器代码，若采样器、着色器资源或可读写资源使用同一个槽，对应的定义应保持完全一致，否则只能使用按槽设置
	HRESULT AddShader(std::string_view name, ID3D11Device* device, ID3DBlob* blob);

	// 编译着色器 或 读取着色器字节码，添加到内部并为其设置标识名
	// 注意：
	// 1. 不同着色器代码，若常量缓冲区使用同一个槽，对应的定义应保持完全一致
	// 2. 不同着色器代码，若存在全局变量，定义应保持完全一致
	// 3. 不同着色器代码，若采样器、着色器资源或可读写资源使用同一个槽，对应的定义应保持完全一致，否则只能使用按槽设置
	HRESULT CreateShaderFromFile(std::string_view name, const std::wstring& filename, ID3D11Device* device,
		LPCSTR entryPoint, LPCSTR shaderModel, const D3D_SHADER_MACRO* pDefines = nullptr, ID3DBlob** ppShaderByteCode = nullptr);

	// 添加带流输出的几何着色器并为其设置标识名
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
	void SetSamplerStateBySlot(UINT slot, ID3D11SamplerState* samplerState);
	// 按名设置采样器状态(若存在同槽多名称则只能使用按槽设置)
	void SetSamplerStateByName(std::string_view name, ID3D11SamplerState* samplerState);
	
	// 按槽设置着色器资源
	void SetShaderResourceBySlot(UINT slot, ID3D11ShaderResourceView* srv);
	// 按名设置着色器资源(若存在同槽多名称则只能使用按槽设置)
	void SetShaderResourceByName(std::string_view name, ID3D11ShaderResourceView* srv);
	

	// 按槽设置可读写资源
	void SetUnorderedAccessBySlot(UINT slot, ID3D11UnorderedAccessView* uav, UINT initialCount);
	// 按名设置可读写资源(若存在同槽多名称则只能使用按槽设置)
	void SetUnorderedAccessByName(std::string_view name, ID3D11UnorderedAccessView* uav, UINT initialCount);

	// 设置调试对象名
	void SetDebugObjectName(std::string name);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};



#endif
