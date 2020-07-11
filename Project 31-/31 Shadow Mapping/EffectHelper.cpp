#include <algorithm>
#include <unordered_map>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include "EffectHelper.h"

using namespace Microsoft::WRL;

# pragma warning(disable: 26812)

//
// 代码宏
//

#define EFFECTHELPER_CREATE_SHADER(FullShaderType, ShaderType)\
{\
	m_##FullShaderType##s[name] = std::make_shared<##FullShaderType##Info>();\
	hr = device->Create##FullShaderType##(blob->GetBufferPointer(), blob->GetBufferSize(),\
		nullptr, m_##FullShaderType##s[name]->p##ShaderType##.GetAddressOf());\
	break;\
}

#define EFFECTHELPER_EFFECTPASS_SET_SHADER_AND_PARAM(FullShaderType, ShaderType) \
{\
	if (pDesc->name##ShaderType##)\
	{\
		auto it = pImpl->m_##FullShaderType##s.find(pDesc->name##ShaderType##);\
		if (it != pImpl->m_##FullShaderType##s.end())\
		{\
			pEffectPass->p##ShaderType##Info = it->second;\
			auto& pCBData = it->second->pParamData;\
			if (pCBData)\
			{\
				pEffectPass->p##ShaderType##ParamData = std::make_unique<CBufferData>(pCBData->cbufferName.c_str(), pCBData->startSlot, pCBData->byteWidth); \
				it->second->pParamData->CreateBuffer(device);\
			}\
		}\
		else\
			return E_INVALIDARG;\
	}\
}

#define EFFECTHELPER_SET_SHADER_DEBUG_NAME(FullShaderType, ShaderType) \
{\
	for (auto& it : pImpl->m_##FullShaderType##s)\
	{\
		std::string name##ShaderType = name + "." + it.first;\
		it.second->p##ShaderType##->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name##ShaderType##.size(), name##ShaderType##.c_str());\
	}\
}

#define EFFECTPASS_SET_SHADER(ShaderType)\
{\
	deviceContext->##ShaderType##SetShader(p##ShaderType##Info->p##ShaderType##.Get(), nullptr, 0);\
}

#define EFFECTPASS_SET_CONSTANTBUFFER(ShaderType)\
{\
	for (UINT slot = 0, mask = p##ShaderType##Info->cbUseMask; mask; ++slot, mask >>= 1)\
	{\
		if (mask & 1)\
		{\
			CBufferData& cbData = cBuffers.at(slot);\
			cbData.UpdateBuffer(deviceContext);\
			deviceContext->##ShaderType##SetConstantBuffers(slot, 1, cBuffers.at(slot).cBuffer.GetAddressOf());\
		}\
	}\
}

#define EFFECTPASS_SET_PARAM(ShaderType)\
{\
	if (!p##ShaderType##Info->params.empty())\
	{\
		if (p##ShaderType##ParamData->isDirty)\
		{\
			p##ShaderType##ParamData->isDirty = false;\
			p##ShaderType##Info->pParamData->isDirty = true;\
			memcpy_s(p##ShaderType##Info->pParamData->pData.get(), p##ShaderType##ParamData->byteWidth,\
				p##ShaderType##ParamData->pData.get(), p##ShaderType##ParamData->byteWidth);\
			p##ShaderType##Info->pParamData->UpdateBuffer(deviceContext);\
		}\
		deviceContext->##ShaderType##SetConstantBuffers(p##ShaderType##Info->pParamData->startSlot,\
			1, p##ShaderType##Info->pParamData->cBuffer.GetAddressOf());\
	}\
}

#define EFFECTPASS_SET_SAMPLER(ShaderType)\
{\
	for (UINT slot = 0, mask = p##ShaderType##Info->ssUseMask; mask; ++slot, mask >>= 1)\
		if (mask & 1)\
			deviceContext->##ShaderType##SetSamplers(slot, 1, samplers.at(slot).pSS.GetAddressOf());\
}

#define EFFECTPASS_SET_SHADERRESOURCE(ShaderType)\
{\
	for (UINT i = 0, slot = i * 32; i < 4; ++i, slot = i * 32)\
		for (UINT mask = p##ShaderType##Info->srUseMasks[i]; mask; ++slot, mask >>= 1)\
			if (mask & 1)\
				deviceContext->##ShaderType##SetShaderResources(slot, 1, shaderResources.at(slot).pSRV.GetAddressOf());\
}

//
// 枚举与类声明
//


enum ShaderFlag {
	PixelShader = 0x1,
	VertexShader = 0x2,
	GeometryShader = 0x4,
	HullShader = 0x8,
	DomainShader = 0x10,
	ComputeShader = 0x20,
};

// 着色器资源
struct ShaderResource
{
	std::string name;
	D3D11_SRV_DIMENSION dim;
	ComPtr<ID3D11ShaderResourceView> pSRV;
};

// 可读写资源
struct RWResource
{
	std::string name;
	D3D11_UAV_DIMENSION dim;
	ComPtr<ID3D11UnorderedAccessView> pUAV;
	UINT initialCount;
	bool enableCounter;
};

// 采样器状态
struct SamplerState
{
	std::string name;
	ComPtr<ID3D11SamplerState> pSS;
};

// 内部使用的常量缓冲区数据
struct CBufferData : CBufferBase
{
	std::unique_ptr<BYTE[]> pData;
	std::string cbufferName;
	UINT startSlot;
	UINT byteWidth;

	CBufferData() : CBufferBase(), startSlot(), byteWidth() {}
	CBufferData(LPCSTR name, UINT startSlot, UINT byteWidth, BYTE* initData = nullptr) :
		CBufferBase(), cbufferName(name), pData(new BYTE[byteWidth]{}), startSlot(startSlot),
		byteWidth(byteWidth)
	{
		if (initData)
			memcpy_s(pData.get(), byteWidth, initData, byteWidth);
	}

	HRESULT CreateBuffer(ID3D11Device* device) override
	{
		if (cBuffer != nullptr)
			return S_OK;
		D3D11_BUFFER_DESC cbd;
		ZeroMemory(&cbd, sizeof(cbd));
		cbd.Usage = D3D11_USAGE_DYNAMIC;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbd.ByteWidth = byteWidth;
		return device->CreateBuffer(&cbd, nullptr, cBuffer.GetAddressOf());
	}

	void UpdateBuffer(ID3D11DeviceContext* deviceContext) override
	{
		if (isDirty)
		{
			isDirty = false;
			D3D11_MAPPED_SUBRESOURCE mappedData;
			deviceContext->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
			memcpy_s(mappedData.pData, byteWidth, pData.get(), byteWidth);
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

struct ConstantBufferVariable : public IEffectConstantBufferVariable
{
	ConstantBufferVariable() = default;
	~ConstantBufferVariable() = default;

	ConstantBufferVariable(UINT offset, UINT size, CBufferData* pData)
		: startByteOffset(offset), byteWidth(size), pCBufferData(pData)
	{
	}

	void SetUInt(UINT val) override
	{
		SetRaw(&val, 0, 4);
	}

	void SetSInt(INT val) override
	{
		SetRaw(&val, 0, 4);
	}

	void SetFloat(FLOAT val) override
	{
		SetRaw(&val, 0, 4);
	}

	void SetUIntVector(UINT numComponents, const UINT data[4]) override
	{
		if (numComponents > 4)
			numComponents = 4;
		UINT byteCount = numComponents * sizeof(UINT);
		if (byteCount > byteWidth)
			byteCount = byteWidth;
		SetRaw(data, 0, byteCount);
	}

	void SetSIntVector(UINT numComponents, const INT data[4]) override
	{
		if (numComponents > 4)
			numComponents = 4;
		UINT byteCount = numComponents * sizeof(INT);
		if (byteCount > byteWidth)
			byteCount = byteWidth;
		SetRaw(data, 0, byteCount);
	}

	void SetFloatVector(UINT numComponents, const FLOAT data[4]) override
	{
		if (numComponents > 4)
			numComponents = 4;
		UINT byteCount = numComponents * sizeof(FLOAT);
		if (byteCount > byteWidth)
			byteCount = byteWidth;
		SetRaw(data, 0, byteCount);
	}

	void SetUIntMatrix(UINT rows, UINT cols, const UINT* noPadData) override
	{
		SetMatrixInBytes(rows, cols, reinterpret_cast<const BYTE*>(noPadData));
	}

	void SetSIntMatrix(UINT rows, UINT cols, const INT* noPadData) override
	{
		SetMatrixInBytes(rows, cols, reinterpret_cast<const BYTE*>(noPadData));
	}

	void SetFloatMatrix(UINT rows, UINT cols, const FLOAT* noPadData) override
	{
		SetMatrixInBytes(rows, cols, reinterpret_cast<const BYTE*>(noPadData));
	}

	void SetRaw(const void* data, UINT byteOffset = 0, UINT byteCount = 0xFFFFFFFF) override
	{
		if (byteOffset > byteWidth)
			return;
		if (byteOffset + byteCount > byteWidth)
			byteCount = byteWidth - byteOffset;

		// 仅当值不同时更新
		if (memcmp(pCBufferData->pData.get() + startByteOffset + byteOffset, data, byteCount))
		{
			memcpy_s(pCBufferData->pData.get() + startByteOffset + byteOffset, byteCount, data, byteCount);
			pCBufferData->isDirty = true;
		}
	}

	HRESULT GetRaw(void* pOutput, UINT byteOffset = 0, UINT byteCount = 0xFFFFFFFF) override
	{
		if (byteOffset > byteWidth || byteCount > byteWidth - byteOffset)
			return E_BOUNDS;
		if (!pOutput)
			return E_INVALIDARG;
		memcpy_s(pOutput, byteCount, pCBufferData->pData.get() + startByteOffset + byteOffset, byteCount);
		return S_OK;
	}

	void SetMatrixInBytes(UINT rows, UINT cols, const BYTE* noPadData)
	{
		// 仅允许1x1到4x4
		if (rows == 0 || rows > 4 || cols == 0 || cols > 4)
			return;
		UINT remainBytes = byteWidth < 64 ? byteWidth : 64;
		BYTE* pData = pCBufferData->pData.get() + startByteOffset;
		while (remainBytes > 0 && rows > 0)
		{
			UINT rowPitch = sizeof(DWORD) * cols < remainBytes ? sizeof(DWORD) * cols : remainBytes;
			// 仅当值不同时更新
			if (memcmp(pData, noPadData, rowPitch))
			{
				memcpy_s(pData, rowPitch, noPadData, rowPitch);
				pCBufferData->isDirty = true;
			}
			noPadData += cols * sizeof(DWORD);
			pData += 16;
			remainBytes = remainBytes < 16 ? 0 : remainBytes - 16;
		}
	}

	UINT startByteOffset = 0;
	UINT byteWidth = 0;
	CBufferData* pCBufferData = nullptr;
};

struct VertexShaderInfo
{
	ComPtr<ID3D11VertexShader> pVS;
	UINT cbUseMask = 0;
	UINT ssUseMask = 0;
	UINT unused = 0;
	UINT srUseMasks[4] = {};
	std::unique_ptr<CBufferData> pParamData = nullptr;
	std::unordered_map<std::string, std::shared_ptr<ConstantBufferVariable>> params;
};

struct DomainShaderInfo
{
	ComPtr<ID3D11DomainShader> pDS;
	UINT cbUseMask = 0;
	UINT ssUseMask = 0;
	UINT unused = 0;
	UINT srUseMasks[4] = {};
	std::unique_ptr<CBufferData> pParamData = nullptr;
	std::unordered_map<std::string, std::shared_ptr<ConstantBufferVariable>> params;
};

struct HullShaderInfo
{
	ComPtr<ID3D11HullShader> pHS;
	UINT cbUseMask = 0;
	UINT ssUseMask = 0;
	UINT unused = 0;
	UINT srUseMasks[4] = {};
	std::unique_ptr<CBufferData> pParamData = nullptr;
	std::unordered_map<std::string, std::shared_ptr<ConstantBufferVariable>> params;
};

struct GeometryShaderInfo
{
	ComPtr<ID3D11GeometryShader> pGS;
	UINT cbUseMask = 0;
	UINT ssUseMask = 0;
	UINT unused = 0;
	UINT srUseMasks[4] = {};
	std::unique_ptr<CBufferData> pParamData = nullptr;
	std::unordered_map<std::string, std::shared_ptr<ConstantBufferVariable>> params;
};

struct PixelShaderInfo
{
	ComPtr<ID3D11PixelShader> pPS;
	UINT cbUseMask = 0;
	UINT ssUseMask = 0;
	UINT rwUseMask = 0;
	UINT srUseMasks[4] = {};
	std::unique_ptr<CBufferData> pParamData = nullptr;
	std::unordered_map<std::string, std::shared_ptr<ConstantBufferVariable>> params;
};

struct ComputeShaderInfo
{
	ComPtr<ID3D11ComputeShader> pCS;
	UINT cbUseMask = 0;
	UINT ssUseMask = 0;
	UINT rwUseMask = 0;
	UINT srUseMasks[4] = {};
	std::unique_ptr<CBufferData> pParamData = nullptr;
	std::unordered_map<std::string, std::shared_ptr<ConstantBufferVariable>> params;
};


struct EffectPass : public IEffectPass
{
	EffectPass(std::unordered_map<UINT, CBufferData>& _cBuffers,
		std::unordered_map<UINT, ShaderResource>& _shaderResources,
		std::unordered_map<UINT, SamplerState>& _samplers,
		std::unordered_map<UINT, RWResource>& _rwResources)
		: cBuffers(_cBuffers), shaderResources(_shaderResources),
		samplers(_samplers), rwResources(_rwResources)
	{
	}

	void SetRasterizerState(ID3D11RasterizerState* pRS)  override;
	void SetBlendState(ID3D11BlendState* pBS, const FLOAT blendFactor[4], UINT sampleMask) override;
	void SetDepthStencilState(ID3D11DepthStencilState* pDSS, UINT stencilRef)  override;
	std::shared_ptr<IEffectConstantBufferVariable> VSGetParamByName(LPCSTR paramName) override;
	std::shared_ptr<IEffectConstantBufferVariable> DSGetParamByName(LPCSTR paramName) override;
	std::shared_ptr<IEffectConstantBufferVariable> HSGetParamByName(LPCSTR paramName) override;
	std::shared_ptr<IEffectConstantBufferVariable> GSGetParamByName(LPCSTR paramName) override;
	std::shared_ptr<IEffectConstantBufferVariable> PSGetParamByName(LPCSTR paramName) override;
	std::shared_ptr<IEffectConstantBufferVariable> CSGetParamByName(LPCSTR paramName) override;
	void Apply(ID3D11DeviceContext* deviceContext) override;

	// 渲染状态
	ComPtr<ID3D11BlendState> pBlendState = nullptr;
	FLOAT blendFactor[4] = {};
	UINT sampleMask = 0xFFFFFFFF;

	ComPtr<ID3D11RasterizerState> pRasterizerState = nullptr;

	ComPtr<ID3D11DepthStencilState> pDepthStencilState = nullptr;
	UINT stencilRef = 0;
	

	// 着色器相关信息
	std::shared_ptr<VertexShaderInfo> pVSInfo = nullptr;
	std::shared_ptr<HullShaderInfo> pHSInfo = nullptr;
	std::shared_ptr<DomainShaderInfo> pDSInfo = nullptr;
	std::shared_ptr<GeometryShaderInfo> pGSInfo = nullptr;
	std::shared_ptr<PixelShaderInfo> pPSInfo = nullptr;
	std::shared_ptr<ComputeShaderInfo> pCSInfo = nullptr;

	// 着色器形参常量缓冲区数据(不创建常量缓冲区)
	std::unique_ptr<CBufferData> pVSParamData = nullptr;
	std::unique_ptr<CBufferData> pHSParamData = nullptr;
	std::unique_ptr<CBufferData> pDSParamData = nullptr;
	std::unique_ptr<CBufferData> pGSParamData = nullptr;
	std::unique_ptr<CBufferData> pPSParamData = nullptr;
	std::unique_ptr<CBufferData> pCSParamData = nullptr;

	// 资源和采样器状态
	std::unordered_map<UINT, CBufferData>& cBuffers;
	std::unordered_map<UINT, ShaderResource>& shaderResources;
	std::unordered_map<UINT, SamplerState>& samplers;
	std::unordered_map<UINT, RWResource>& rwResources;
};

class EffectHelper::Impl
{
public:
	Impl() { Clear(); }
	~Impl() = default;

	// 更新收集着色器反射信息
	HRESULT UpdateShaderReflection(LPCSTR name, ID3D11Device* device, ID3D11ShaderReflection* pShaderReflection, UINT shaderFlag);
	// 清空所有资源与反射信息
	void Clear();
	//根据Blob创建着色器并指定标识名
	HRESULT CreateShaderFromBlob(LPCSTR name, ID3D11Device* device, UINT shaderFlag,
		ID3DBlob* blob);

public:
	std::unordered_map<std::string, std::shared_ptr<EffectPass>> m_EffectPasses;				// 渲染通道

	std::unordered_map<std::string,																// 常量缓冲区的变量
		std::shared_ptr<ConstantBufferVariable>> m_ConstantBufferVariables;
	std::unordered_map<UINT, CBufferData> m_CBuffers;											// 常量缓冲区临时缓存的数据
	std::unordered_map<UINT, ShaderResource> m_ShaderResources;									// 着色器资源
	std::unordered_map<UINT, SamplerState> m_Samplers;											// 采样器
	std::unordered_map<UINT, RWResource> m_RWResources;											// 可读写资源

	std::unordered_map<std::string, std::shared_ptr<VertexShaderInfo>> m_VertexShaders;			// 顶点着色器
	std::unordered_map<std::string, std::shared_ptr<HullShaderInfo>> m_HullShaders;				// 外壳着色器
	std::unordered_map<std::string, std::shared_ptr<DomainShaderInfo>> m_DomainShaders;			// 域着色器
	std::unordered_map<std::string, std::shared_ptr<GeometryShaderInfo>> m_GeometryShaders;		// 几何着色器
	std::unordered_map<std::string, std::shared_ptr<PixelShaderInfo>> m_PixelShaders;			// 像素着色器
	std::unordered_map<std::string, std::shared_ptr<ComputeShaderInfo>> m_ComputeShaders;		// 计算着色器


													
};

//
// EffectHelper::Impl
//


HRESULT EffectHelper::Impl::UpdateShaderReflection(LPCSTR name, ID3D11Device* device, ID3D11ShaderReflection* pShaderReflection, UINT shaderFlag)
{
	HRESULT hr;

	D3D11_SHADER_DESC sd;
	hr = pShaderReflection->GetDesc(&sd);
	if (FAILED(hr))
		return hr;

	for (UINT i = 0;; ++i)
	{
		D3D11_SHADER_INPUT_BIND_DESC sibDesc;
		hr = pShaderReflection->GetResourceBindingDesc(i, &sibDesc);
		// 读取完变量后会失败，但这并不是失败的调用
		if (FAILED(hr))
			break;

		// 常量缓冲区
		if (sibDesc.Type == D3D_SIT_CBUFFER)
		{
			ID3D11ShaderReflectionConstantBuffer* pSRCBuffer = pShaderReflection->GetConstantBufferByName(sibDesc.Name);
			// 获取cbuffer内的变量信息并建立映射
			D3D11_SHADER_BUFFER_DESC cbDesc{};
			hr = pSRCBuffer->GetDesc(&cbDesc);
			if (FAILED(hr))
				return hr;

			bool isParam = !strcmp(sibDesc.Name, "$Params");

			// 确定常量缓冲区的创建位置
			if (!isParam)
			{
				auto it = m_CBuffers.find(sibDesc.BindPoint);
				if (it == m_CBuffers.end())
				{
					m_CBuffers.emplace(std::make_pair(sibDesc.BindPoint, CBufferData(sibDesc.Name, sibDesc.BindPoint, cbDesc.Size, nullptr)));
					m_CBuffers[sibDesc.BindPoint].CreateBuffer(device);
				}

				// 标记该着色器使用了当前常量缓冲区
				if (cbDesc.Variables > 0)
				{
					switch (shaderFlag)
					{
					case VertexShader: m_VertexShaders[name]->cbUseMask |= (1 << sibDesc.BindPoint); break;
					case DomainShader: m_DomainShaders[name]->cbUseMask |= (1 << sibDesc.BindPoint); break;
					case HullShader: m_HullShaders[name]->cbUseMask |= (1 << sibDesc.BindPoint); break;
					case GeometryShader: m_GeometryShaders[name]->cbUseMask |= (1 << sibDesc.BindPoint); break;
					case PixelShader: m_PixelShaders[name]->cbUseMask |= (1 << sibDesc.BindPoint); break;
					case ComputeShader: m_ComputeShaders[name]->cbUseMask |= (1 << sibDesc.BindPoint); break;
					}
				}
			}
			else if (cbDesc.Variables > 0)
			{
				switch (shaderFlag)
				{
				case VertexShader: m_VertexShaders[name]->pParamData = std::make_unique<CBufferData>(sibDesc.Name, sibDesc.BindPoint, cbDesc.Size, nullptr); break;
				case DomainShader: m_DomainShaders[name]->pParamData = std::make_unique<CBufferData>(sibDesc.Name, sibDesc.BindPoint, cbDesc.Size, nullptr); break;
				case HullShader: m_HullShaders[name]->pParamData = std::make_unique<CBufferData>(sibDesc.Name, sibDesc.BindPoint, cbDesc.Size, nullptr); break;
				case GeometryShader: m_GeometryShaders[name]->pParamData = std::make_unique<CBufferData>(sibDesc.Name, sibDesc.BindPoint, cbDesc.Size, nullptr); break;
				case PixelShader: m_PixelShaders[name]->pParamData = std::make_unique<CBufferData>(sibDesc.Name, sibDesc.BindPoint, cbDesc.Size, nullptr); break;
				case ComputeShader: m_ComputeShaders[name]->pParamData = std::make_unique<CBufferData>(sibDesc.Name, sibDesc.BindPoint, cbDesc.Size, nullptr); break;
				}
			}

			// 记录内部变量
			for (UINT j = 0; j < cbDesc.Variables; ++j)
			{
				ID3D11ShaderReflectionVariable* pSRVar = pSRCBuffer->GetVariableByIndex(j);
				D3D11_SHADER_VARIABLE_DESC svDesc;
				hr = pSRVar->GetDesc(&svDesc);
				if (FAILED(hr))
					return hr;

				// 着色器形参需要特殊对待
				// 记录着色器的uniform形参
				// **忽略着色器形参默认值**
				if (isParam)
				{
					switch (shaderFlag)
					{
					case VertexShader: m_VertexShaders[name]->params[svDesc.Name] =
						std::make_shared<ConstantBufferVariable>(svDesc.StartOffset, svDesc.Size, m_VertexShaders[name]->pParamData.get());
						break;
					case DomainShader: m_DomainShaders[name]->params[svDesc.Name] =
						std::make_shared<ConstantBufferVariable>(svDesc.StartOffset, svDesc.Size, m_DomainShaders[name]->pParamData.get());
						break;
					case HullShader: m_HullShaders[name]->params[svDesc.Name] =
						std::make_shared<ConstantBufferVariable>(svDesc.StartOffset, svDesc.Size, m_HullShaders[name]->pParamData.get());
						break;
					case GeometryShader: m_GeometryShaders[name]->params[svDesc.Name] =
						std::make_shared<ConstantBufferVariable>(svDesc.StartOffset, svDesc.Size, m_GeometryShaders[name]->pParamData.get());
						break;
					case PixelShader: m_PixelShaders[name]->params[svDesc.Name] =
						std::make_shared<ConstantBufferVariable>(svDesc.StartOffset, svDesc.Size, m_PixelShaders[name]->pParamData.get());
						break;
					case ComputeShader: m_ComputeShaders[name]->params[svDesc.Name] =
						std::make_shared<ConstantBufferVariable>(svDesc.StartOffset, svDesc.Size, m_ComputeShaders[name]->pParamData.get());
						break;
					}
				}
				// 常量缓冲区的成员
				else
				{	
					m_ConstantBufferVariables[svDesc.Name] = std::make_shared<ConstantBufferVariable>(
						svDesc.StartOffset, svDesc.Size, &m_CBuffers[sibDesc.BindPoint]);
					// 如果有默认值，对其赋初值
					if (svDesc.DefaultValue)
						m_ConstantBufferVariables[svDesc.Name]->SetRaw(svDesc.DefaultValue);
				}
			}
			
			

			
			
		}
		// 着色器资源
		else if (sibDesc.Type == D3D_SIT_TEXTURE || sibDesc.Type == D3D_SIT_STRUCTURED || sibDesc.Type == D3D_SIT_BYTEADDRESS ||
			sibDesc.Type == D3D_SIT_TBUFFER)
		{
			auto it = m_ShaderResources.find(sibDesc.BindPoint);
			if (it == m_ShaderResources.end())
			{
				m_ShaderResources.emplace(std::make_pair(sibDesc.BindPoint,
					ShaderResource{ sibDesc.Name, sibDesc.Dimension, nullptr }));
			}
			
			// 标记该着色器使用了当前着色器资源
			switch (shaderFlag)
			{
			case VertexShader: m_VertexShaders[name]->srUseMasks[sibDesc.BindPoint / 32] |= (1 << (sibDesc.BindPoint % 32)); break;
			case DomainShader: m_DomainShaders[name]->srUseMasks[sibDesc.BindPoint / 32] |= (1 << (sibDesc.BindPoint % 32)); break;
			case HullShader: m_HullShaders[name]->srUseMasks[sibDesc.BindPoint / 32] |= (1 << (sibDesc.BindPoint % 32)); break;
			case GeometryShader: m_GeometryShaders[name]->srUseMasks[sibDesc.BindPoint / 32] |= (1 << (sibDesc.BindPoint % 32)); break;
			case PixelShader: m_PixelShaders[name]->srUseMasks[sibDesc.BindPoint / 32] |= (1 << (sibDesc.BindPoint % 32)); break;
			case ComputeShader: m_ComputeShaders[name]->srUseMasks[sibDesc.BindPoint / 32] |= (1 << (sibDesc.BindPoint % 32)); break;
			}

		}
		// 采样器
		else if (sibDesc.Type == D3D_SIT_SAMPLER)
		{
			auto it = m_Samplers.find(sibDesc.BindPoint);
			if (it == m_Samplers.end())
			{
				m_Samplers.emplace(std::make_pair(sibDesc.BindPoint,
					SamplerState{ sibDesc.Name, nullptr }));
			}
			
			// 标记该着色器使用了当前采样器
			switch (shaderFlag)
			{
			case VertexShader: m_VertexShaders[name]->ssUseMask |= (1 << sibDesc.BindPoint); break;
			case DomainShader: m_DomainShaders[name]->ssUseMask |= (1 << sibDesc.BindPoint); break;
			case HullShader: m_HullShaders[name]->ssUseMask |= (1 << sibDesc.BindPoint); break;
			case GeometryShader: m_GeometryShaders[name]->ssUseMask |= (1 << sibDesc.BindPoint); break;
			case PixelShader: m_PixelShaders[name]->ssUseMask |= (1 << sibDesc.BindPoint); break;
			case ComputeShader: m_ComputeShaders[name]->ssUseMask |= (1 << sibDesc.BindPoint); break;
			}

		}
		// 可读写资源
		else if (sibDesc.Type == D3D_SIT_UAV_RWTYPED || sibDesc.Type == D3D_SIT_UAV_RWSTRUCTURED ||
			sibDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER || sibDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED ||
			sibDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED || sibDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
		{
			auto it = m_RWResources.find(sibDesc.BindPoint);
			if (it == m_RWResources.end())
			{
				m_RWResources.emplace(std::make_pair(sibDesc.BindPoint,
					RWResource{ sibDesc.Name, static_cast<D3D11_UAV_DIMENSION>(sibDesc.Dimension), nullptr, 0, false }));
			}

			// 标记该着色器使用了当前可读写资源
			switch (shaderFlag)
			{
			case PixelShader: m_PixelShaders[name]->rwUseMask |= (1 << sibDesc.BindPoint); break;
			case ComputeShader: m_ComputeShaders[name]->rwUseMask |= (1 << sibDesc.BindPoint); break;
			}
		}
	}

	return S_OK;
}

void EffectHelper::Impl::Clear()
{

	m_CBuffers.clear();

	m_ConstantBufferVariables.clear();
	m_ShaderResources.clear();
	m_Samplers.clear();
	m_RWResources.clear();

	m_VertexShaders.clear();
	m_HullShaders.clear();
	m_DomainShaders.clear();
	m_GeometryShaders.clear();
	m_PixelShaders.clear();
	m_ComputeShaders.clear();
}

HRESULT EffectHelper::Impl::CreateShaderFromBlob(LPCSTR name, ID3D11Device* device, UINT shaderFlag,
	ID3DBlob* blob)
{
	HRESULT hr = 0;
	ComPtr<ID3D11VertexShader> pVS;
	ComPtr<ID3D11DomainShader> pDS;
	ComPtr<ID3D11HullShader> pHS;
	ComPtr<ID3D11GeometryShader> pGS;
	ComPtr<ID3D11PixelShader> pPS;
	ComPtr<ID3D11ComputeShader> pCS;
	// 创建着色器
	switch (shaderFlag)
	{
	case PixelShader: EFFECTHELPER_CREATE_SHADER(PixelShader, PS);
	case VertexShader: EFFECTHELPER_CREATE_SHADER(VertexShader, VS);
	case GeometryShader: EFFECTHELPER_CREATE_SHADER(GeometryShader, GS);
	case HullShader: EFFECTHELPER_CREATE_SHADER(HullShader, HS);
	case DomainShader: EFFECTHELPER_CREATE_SHADER(DomainShader, DS);
	case ComputeShader: EFFECTHELPER_CREATE_SHADER(ComputeShader, CS);
	}

	return hr;
}

//
// EffectHelper
//

EffectHelper::EffectHelper()
	: pImpl(std::make_unique<EffectHelper::Impl>())
{
}

EffectHelper::~EffectHelper()
{
}

HRESULT EffectHelper::AddShader(LPCSTR name, ID3D11Device* device, ID3DBlob* blob)
{
	if (!name)
		return E_INVALIDARG;
	
	HRESULT hr;

	// 着色器反射
	ComPtr<ID3D11ShaderReflection> pShaderReflection;
	hr = D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D11ShaderReflection),
		reinterpret_cast<void**>(pShaderReflection.GetAddressOf()));
	if (FAILED(hr))
		return hr;

	// 获取着色器类型
	D3D11_SHADER_DESC sd;
	pShaderReflection->GetDesc(&sd);
	UINT shaderFlag = static_cast<ShaderFlag>(1 << D3D11_SHVER_GET_TYPE(sd.Version));

	// 创建着色器
	hr = pImpl->CreateShaderFromBlob(name, device, shaderFlag, blob);
	if (FAILED(hr))
		return hr;

	// 建立着色器反射
	return pImpl->UpdateShaderReflection(name, device, pShaderReflection.Get(), shaderFlag);
}

void EffectHelper::Clear()
{
	pImpl->Clear();
}

HRESULT EffectHelper::AddEffectPass(LPCSTR effectPassName, ID3D11Device* device, EffectPassDesc* pDesc)
{
	if (!pDesc || !effectPassName)
		return E_INVALIDARG;

	// 不允许重复添加
	auto it = pImpl->m_EffectPasses.find(effectPassName);
	if (it != pImpl->m_EffectPasses.end())
		return ERROR_OBJECT_NAME_EXISTS;

	auto pEffectPass = pImpl->m_EffectPasses[effectPassName] = 
		std::make_shared<EffectPass>(pImpl->m_CBuffers, pImpl->m_ShaderResources, pImpl->m_Samplers, pImpl->m_RWResources);

	EFFECTHELPER_EFFECTPASS_SET_SHADER_AND_PARAM(VertexShader, VS);
	EFFECTHELPER_EFFECTPASS_SET_SHADER_AND_PARAM(DomainShader, DS);
	EFFECTHELPER_EFFECTPASS_SET_SHADER_AND_PARAM(HullShader, HS);
	EFFECTHELPER_EFFECTPASS_SET_SHADER_AND_PARAM(GeometryShader, GS);
	EFFECTHELPER_EFFECTPASS_SET_SHADER_AND_PARAM(PixelShader, PS);
	EFFECTHELPER_EFFECTPASS_SET_SHADER_AND_PARAM(ComputeShader, CS);
		
	return S_OK;
}

std::shared_ptr<IEffectPass> EffectHelper::GetEffectPass(LPCSTR effectPassName)
{
	auto it = pImpl->m_EffectPasses.find(effectPassName);
	if (it != pImpl->m_EffectPasses.end())
		return it->second;
	return nullptr;
}

std::shared_ptr<IEffectConstantBufferVariable> EffectHelper::GetConstantBufferVariable(LPCSTR name)
{
	auto it = pImpl->m_ConstantBufferVariables.find(name);
	if (it != pImpl->m_ConstantBufferVariables.end())
		return it->second;
	else
		return nullptr;
}

void EffectHelper::SetSamplerStateBySlot(UINT slot, ID3D11SamplerState* samplerState)
{
	auto it = pImpl->m_Samplers.find(slot);
	if (it != pImpl->m_Samplers.end())
		it->second.pSS = samplerState;
}

void EffectHelper::SetSamplerStateByName(LPCSTR name, ID3D11SamplerState* samplerState)
{
	auto it = std::find_if(pImpl->m_Samplers.begin(), pImpl->m_Samplers.end(),
		[name](const std::pair<UINT, SamplerState>& p) {
			return p.second.name == name;
		});
	if (it != pImpl->m_Samplers.end())
		it->second.pSS = samplerState;
}

void EffectHelper::SetShaderResourceBySlot(UINT slot, ID3D11ShaderResourceView* srv)
{
	auto it = pImpl->m_ShaderResources.find(slot);
	if (it != pImpl->m_ShaderResources.end())
		it->second.pSRV = srv;
}

void EffectHelper::SetShaderResourceByName(LPCSTR name, ID3D11ShaderResourceView* srv)
{
	auto it = std::find_if(pImpl->m_ShaderResources.begin(), pImpl->m_ShaderResources.end(),
		[name](const std::pair<UINT, ShaderResource>& p) {
			return p.second.name == name;
		});
	if (it != pImpl->m_ShaderResources.end())
		it->second.pSRV = srv;
}

void EffectHelper::SetUnorderedAccessBySlot(UINT slot, ID3D11UnorderedAccessView* uav, UINT* pUAVInitialCount)
{
	auto it = pImpl->m_RWResources.find(slot);
	if (it != pImpl->m_RWResources.end())
		it->second.pUAV = uav;
}

void EffectHelper::SetUnorderedAccessByName(LPCSTR name, ID3D11UnorderedAccessView* uav, UINT* pUAVInitialCount)
{
	auto it = std::find_if(pImpl->m_RWResources.begin(), pImpl->m_RWResources.end(),
		[name](const std::pair<UINT, RWResource>& p) {
			return p.second.name == name;
		});
	if (it != pImpl->m_RWResources.end())
		it->second.pUAV = uav;
}

void EffectHelper::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	for (auto& it : pImpl->m_CBuffers)
	{
		std::string cBufferName = name + "." + it.second.cbufferName;
		it.second.cBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)cBufferName.size(), cBufferName.c_str());
	}

	EFFECTHELPER_SET_SHADER_DEBUG_NAME(VertexShader, VS);
	EFFECTHELPER_SET_SHADER_DEBUG_NAME(DomainShader, DS);
	EFFECTHELPER_SET_SHADER_DEBUG_NAME(HullShader, HS);
	EFFECTHELPER_SET_SHADER_DEBUG_NAME(GeometryShader, GS);
	EFFECTHELPER_SET_SHADER_DEBUG_NAME(PixelShader, PS);
	EFFECTHELPER_SET_SHADER_DEBUG_NAME(ComputeShader, CS);

#else
	UNREFERENCED_PARAMETER(name);
#endif
}

//
// EffectPass
//

void EffectPass::SetRasterizerState(ID3D11RasterizerState* pRS)
{
	pRasterizerState = pRS;
}

void EffectPass::SetBlendState(ID3D11BlendState* pBS, const FLOAT blendFactor[4], UINT sampleMask)
{
	pBlendState = pBS;
	if (blendFactor)
		memcpy_s(this->blendFactor, sizeof(FLOAT[4]), blendFactor, sizeof(FLOAT[4]));
	this->sampleMask = sampleMask;
}

void EffectPass::SetDepthStencilState(ID3D11DepthStencilState* pDSS, UINT stencilRef)
{
	pDepthStencilState = pDSS;
	this->stencilRef = stencilRef;
}

std::shared_ptr<IEffectConstantBufferVariable> EffectPass::VSGetParamByName(LPCSTR paramName)
{
	if (pVSInfo)
	{
		auto it = pVSInfo->params.find(paramName);
		if (it != pVSInfo->params.end())
			return std::make_shared<ConstantBufferVariable>(it->second->startByteOffset, it->second->byteWidth, pVSParamData.get());
	}
	return nullptr;
}

std::shared_ptr<IEffectConstantBufferVariable> EffectPass::DSGetParamByName(LPCSTR paramName)
{
	if (pDSInfo)
	{
		auto it = pDSInfo->params.find(paramName);
		if (it != pDSInfo->params.end())
			return std::make_shared<ConstantBufferVariable>(it->second->startByteOffset, it->second->byteWidth, pDSParamData.get());
	}
	return nullptr;
}

std::shared_ptr<IEffectConstantBufferVariable> EffectPass::HSGetParamByName(LPCSTR paramName)
{
	if (pHSInfo)
	{
		auto it = pHSInfo->params.find(paramName);
		if (it != pHSInfo->params.end())
			return std::make_shared<ConstantBufferVariable>(it->second->startByteOffset, it->second->byteWidth, pHSParamData.get());
	}
	return nullptr;
}

std::shared_ptr<IEffectConstantBufferVariable> EffectPass::GSGetParamByName(LPCSTR paramName)
{
	if (pGSInfo)
	{
		auto it = pGSInfo->params.find(paramName);
		if (it != pGSInfo->params.end())
			return std::make_shared<ConstantBufferVariable>(it->second->startByteOffset, it->second->byteWidth, pGSParamData.get());
	}
	return nullptr;
}

std::shared_ptr<IEffectConstantBufferVariable> EffectPass::PSGetParamByName(LPCSTR paramName)
{
	if (pPSInfo)
	{
		auto it = pPSInfo->params.find(paramName);
		if (it != pPSInfo->params.end())
			return std::make_shared<ConstantBufferVariable>(it->second->startByteOffset, it->second->byteWidth, pPSParamData.get());
	}
	return nullptr;
}

std::shared_ptr<IEffectConstantBufferVariable> EffectPass::CSGetParamByName(LPCSTR paramName)
{
	if (pCSInfo)
	{
		auto it = pCSInfo->params.find(paramName);
		if (it != pCSInfo->params.end())
			return std::make_shared<ConstantBufferVariable>(it->second->startByteOffset, it->second->byteWidth, pCSParamData.get());
	}
	return nullptr;
}

void EffectPass::Apply(ID3D11DeviceContext* deviceContext)
{
	//
	// 设置着色器、常量缓冲区、形参常量缓冲区、采样器、着色器资源、可读写资源
	//
	if (pVSInfo)
	{
		EFFECTPASS_SET_SHADER(VS);
		EFFECTPASS_SET_CONSTANTBUFFER(VS);
		EFFECTPASS_SET_PARAM(VS);
		EFFECTPASS_SET_SAMPLER(VS);
		EFFECTPASS_SET_SHADERRESOURCE(VS);
	}
	else
	{
		deviceContext->VSSetShader(nullptr, nullptr, 0);
	}
	
	if (pDSInfo)
	{
		EFFECTPASS_SET_SHADER(DS);
		EFFECTPASS_SET_CONSTANTBUFFER(DS);
		EFFECTPASS_SET_PARAM(DS);
		EFFECTPASS_SET_SAMPLER(DS);
		EFFECTPASS_SET_SHADERRESOURCE(DS);
	}
	else
	{
		deviceContext->DSSetShader(nullptr, nullptr, 0);
	}

	if (pHSInfo)
	{
		EFFECTPASS_SET_SHADER(HS);
		EFFECTPASS_SET_CONSTANTBUFFER(HS);
		EFFECTPASS_SET_PARAM(HS);
		EFFECTPASS_SET_SAMPLER(HS);
		EFFECTPASS_SET_SHADERRESOURCE(HS);
	}
	else
	{
		deviceContext->HSSetShader(nullptr, nullptr, 0);
	}

	if (pGSInfo)
	{
		EFFECTPASS_SET_SHADER(GS);
		EFFECTPASS_SET_CONSTANTBUFFER(GS);
		EFFECTPASS_SET_PARAM(GS);
		EFFECTPASS_SET_SAMPLER(GS);
		EFFECTPASS_SET_SHADERRESOURCE(GS);
	}
	else
	{
		deviceContext->GSSetShader(nullptr, nullptr, 0);
	}
	
	if (pPSInfo)
	{
		EFFECTPASS_SET_SHADER(PS);
		EFFECTPASS_SET_CONSTANTBUFFER(PS);
		EFFECTPASS_SET_PARAM(PS);
		EFFECTPASS_SET_SAMPLER(PS);
		EFFECTPASS_SET_SHADERRESOURCE(PS);
		for (UINT slot = 0, mask = pPSInfo->rwUseMask; mask; ++slot, mask >>= 1)
		{
			if (mask & 1)
			{
				auto& res = rwResources.at(slot);
				deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
					nullptr, nullptr, slot, 1, res.pUAV.GetAddressOf(),
					(res.enableCounter ? &res.initialCount : nullptr));
			}
		}
	}
	else
	{
		deviceContext->PSSetShader(nullptr, nullptr, 0);
	}
	
	if (pCSInfo)
	{
		EFFECTPASS_SET_SHADER(CS);
		EFFECTPASS_SET_CONSTANTBUFFER(CS);
		EFFECTPASS_SET_PARAM(CS);
		EFFECTPASS_SET_SAMPLER(CS);
		EFFECTPASS_SET_SHADERRESOURCE(CS);
		for (UINT slot = 0, mask = pCSInfo->rwUseMask; mask; ++slot, mask >>= 1)
		{
			if (mask & 1)
			{
				auto& res = rwResources.at(slot);
				deviceContext->CSSetUnorderedAccessViews(slot, 1, res.pUAV.GetAddressOf(),
					(res.enableCounter ? &res.initialCount : nullptr));
			}
		}
	}
	else
	{
		deviceContext->CSSetShader(nullptr, nullptr, 0);
	}

	// 设置渲染状态
	deviceContext->RSSetState(pRasterizerState.Get());
	deviceContext->OMSetBlendState(pBlendState.Get(), blendFactor, sampleMask);
	deviceContext->OMSetDepthStencilState(pDepthStencilState.Get(), stencilRef);
}
