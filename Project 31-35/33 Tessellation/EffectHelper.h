//***************************************************************************************
// EffectsHelper.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 定义一些实用的特效助手类
// Define utility effect helper classes.
//***************************************************************************************

//
// 将EffectHelper.h和EffectHelper.cpp包含进项目即可使用
// 

#ifndef EFFECTHELPER_H
#define EFFECTHELPER_H

//
// 宏相关
//

// 默认开启图形调试器具名化
// 如果不需要该项功能，可通过全局文本替换将其值设置为0
#if (defined(DEBUG) || defined(_DEBUG))
	#ifndef GRAPHICS_DEBUGGER_OBJECT_NAME
	#define GRAPHICS_DEBUGGER_OBJECT_NAME 1
	#endif
#endif

#include <string>
#include <memory>
#include <d3d11_1.h>
#include <wrl/client.h>

// 若类需要内存对齐，从该类派生
template<class DerivedType>
struct AlignedType
{
	static void* operator new(size_t size)
	{
		const size_t alignedSize = __alignof(DerivedType);

		static_assert(alignedSize > 8, "AlignedNew is only useful for types with > 8 byte alignment! Did you forget a __declspec(align) on DerivedType?");

		void* ptr = _aligned_malloc(size, alignedSize);

		if (!ptr)
			throw std::bad_alloc();

		return ptr;
	}

	static void operator delete(void* ptr)
	{
		_aligned_free(ptr);
	}
};

struct CBufferBase
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	CBufferBase() : isDirty() {}

	BOOL isDirty;
	ComPtr<ID3D11Buffer> cBuffer;

	virtual HRESULT CreateBuffer(ID3D11Device* device) = 0;
	virtual void UpdateBuffer(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindVS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindHS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindDS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindGS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindCS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindPS(ID3D11DeviceContext* deviceContext) = 0;
};

// 用于创建简易Effect框架
template<UINT startSlot, class T>
struct CBufferObject : CBufferBase
{
	T data;

	CBufferObject() : CBufferBase(), data() {}

	HRESULT CreateBuffer(ID3D11Device* device) override
	{
		if (cBuffer != nullptr)
			return S_OK;
		D3D11_BUFFER_DESC cbd;
		ZeroMemory(&cbd, sizeof(cbd));
		cbd.Usage = D3D11_USAGE_DYNAMIC;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbd.ByteWidth = sizeof(T);
		return device->CreateBuffer(&cbd, nullptr, cBuffer.GetAddressOf());
	}

	void UpdateBuffer(ID3D11DeviceContext* deviceContext) override
	{
		if (isDirty)
		{
			isDirty = false;
			D3D11_MAPPED_SUBRESOURCE mappedData;
			deviceContext->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
			memcpy_s(mappedData.pData, sizeof(T), &data, sizeof(T));
			deviceContext->Unmap(cBuffer.Get(), 0);
		}
	}

	void BindVS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->VSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindHS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->HSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindDS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->DSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindGS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->GSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindCS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->CSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindPS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->PSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}
};

//
// EffectHelper
//

// 渲染通道描述
// 通过指定添加着色器时提供的名字来设置着色器
struct EffectPassDesc
{
	LPCSTR nameVS = nullptr;
	LPCSTR nameDS = nullptr;
	LPCSTR nameHS = nullptr;
	LPCSTR nameGS = nullptr;
	LPCSTR namePS = nullptr;
	LPCSTR nameCS = nullptr;
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

	// 获取最近一次设置的值，允许指定读取范围
	virtual HRESULT GetRaw(void* pOutput, UINT byteOffset = 0, UINT byteCount = 0xFFFFFFFF) = 0;
};

// 渲染通道
// 非COM组件
struct IEffectPass
{
	// 设置光栅化状态
	virtual void SetRasterizerState(ID3D11RasterizerState* pRS) = 0;
	// 设置混合状态
	virtual void SetBlendState(ID3D11BlendState* pBS, const FLOAT blendFactor[4], UINT sampleMask) = 0;
	// 设置深度混合状态
	virtual void SetDepthStencilState(ID3D11DepthStencilState* pDSS, UINT stencilValue) = 0;
	// 获取顶点着色器的uniform形参用于设置值
	virtual std::shared_ptr<IEffectConstantBufferVariable> VSGetParamByName(LPCSTR paramName) = 0;
	// 获取域着色器的uniform形参用于设置值
	virtual std::shared_ptr<IEffectConstantBufferVariable> DSGetParamByName(LPCSTR paramName) = 0;
	// 获取外壳着色器的uniform形参用于设置值
	virtual std::shared_ptr<IEffectConstantBufferVariable> HSGetParamByName(LPCSTR paramName) = 0;
	// 获取几何着色器的uniform形参用于设置值
	virtual std::shared_ptr<IEffectConstantBufferVariable> GSGetParamByName(LPCSTR paramName) = 0;
	// 获取像素着色器的uniform形参用于设置值
	virtual std::shared_ptr<IEffectConstantBufferVariable> PSGetParamByName(LPCSTR paramName) = 0;
	// 获取计算着色器的uniform形参用于设置值
	virtual std::shared_ptr<IEffectConstantBufferVariable> CSGetParamByName(LPCSTR paramName) = 0;
	// 应用着色器、常量缓冲区(包括函数形参)、采样器、着色器资源和可读写资源到渲染管线
	virtual void Apply(ID3D11DeviceContext* deviceContext) = 0;
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
	HRESULT AddShader(LPCSTR name, ID3D11Device* device, ID3DBlob* blob);

	// 添加带流输出的几何着色器并为其设置标识名
	// 注意：
	// 1. 不同着色器代码，若常量缓冲区使用同一个槽，对应的定义应保持完全一致
	// 2. 不同着色器代码，若存在全局变量，定义应保持完全一致
	// 3. 不同着色器代码，若采样器、着色器资源或可读写资源使用同一个槽，对应的定义应保持完全一致，否则只能使用按槽设置 
	HRESULT AddGeometryShaderWithStreamOutput(LPCSTR name, ID3D11Device* device, ID3D11GeometryShader* gsWithSO, ID3DBlob* blob);

	// 清空所有内容
	void Clear();

	// 创建渲染通道
	HRESULT AddEffectPass(LPCSTR effectPassName, ID3D11Device* device, EffectPassDesc* pDesc);
	// 获取特定渲染通道
	std::shared_ptr<IEffectPass> GetEffectPass(LPCSTR effectPassName);

	// 获取常量缓冲区的变量用于设置值
	std::shared_ptr<IEffectConstantBufferVariable> GetConstantBufferVariable(LPCSTR name);

	// 按槽设置采样器状态
	void SetSamplerStateBySlot(UINT slot, ID3D11SamplerState* samplerState);
	// 按名设置采样器状态(若存在同槽多名称则只能使用按槽设置)
	void SetSamplerStateByName(LPCSTR name, ID3D11SamplerState* samplerState);
	
	// 按槽设置着色器资源
	void SetShaderResourceBySlot(UINT slot, ID3D11ShaderResourceView* srv);
	// 按名设置着色器资源(若存在同槽多名称则只能使用按槽设置)
	void SetShaderResourceByName(LPCSTR name, ID3D11ShaderResourceView* srv);

	// 按槽设置可读写资源
	void SetUnorderedAccessBySlot(UINT slot, ID3D11UnorderedAccessView* uav, UINT initialCount);
	// 按名设置可读写资源(若存在同槽多名称则只能使用按槽设置)
	void SetUnorderedAccessByName(LPCSTR name, ID3D11UnorderedAccessView* uav, UINT initialCount);

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};



#endif
