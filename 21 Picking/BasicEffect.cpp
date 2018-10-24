#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;

//
// 这些结构体对应HLSL的结构体，仅供该文件使用。需要按16字节对齐
//

struct CBChangesEveryObjectDrawing
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
};

struct CBChangesEveryInstanceDrawing
{
	Material material;
};

struct CBDrawingStates
{
	int textureUsed;
	DirectX::XMFLOAT3 pad;
};

struct CBChangesEveryFrame
{
	DirectX::XMMATRIX view;
	DirectX::XMVECTOR eyePos;
};

struct CBChangesOnResize
{
	DirectX::XMMATRIX proj;
};

struct CBChangesRarely
{
	DirectionalLight dirLight[BasicEffect::maxLights];
	PointLight pointLight[BasicEffect::maxLights];
	SpotLight spotLight[BasicEffect::maxLights];
};


//
// BasicEffect::Impl 需要先于BasicEffect的定义
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:
	// 必须显式指定
	Impl() = default;
	~Impl() = default;

	// objFileNameInOut为编译好的着色器二进制文件(.*so)，若有指定则优先寻找该文件并读取
	// hlslFileName为着色器代码，若未找到着色器二进制文件则编译着色器代码
	// 编译成功后，若指定了objFileNameInOut，则保存编译好的着色器二进制信息到该文件
	// ppBlobOut输出着色器二进制信息
	HRESULT CreateShaderFromFile(const WCHAR* objFileNameInOut, const WCHAR* hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppBlobOut);

public:
	// 需要16字节对齐的优先放在前面
	CBufferObject<0, CBChangesEveryObjectDrawing>	cbObjDrawing;		// 每次对象绘制的常量缓冲区
	CBufferObject<1, CBChangesEveryInstanceDrawing> cbInstDrawing;		// 每次实例绘制的常量缓冲区
	CBufferObject<2, CBDrawingStates>				cbStates;			// 每次绘制状态改变的常量缓冲区
	CBufferObject<3, CBChangesEveryFrame>			cbFrame;			// 每帧绘制的常量缓冲区
	CBufferObject<4, CBChangesOnResize>				cbOnResize;			// 每次窗口大小变更的常量缓冲区
	CBufferObject<5, CBChangesRarely>				cbRarely;			// 几乎不会变更的常量缓冲区
	BOOL isDirty;											// 是否有值变更
	std::vector<CBufferBase*> cBufferPtrs;					// 统一管理上面所有的常量缓冲区


	ComPtr<ID3D11VertexShader> basicInstanceVS;
	ComPtr<ID3D11VertexShader> basicObjectVS;

	ComPtr<ID3D11PixelShader> basicPS;

	ComPtr<ID3D11InputLayout> instancePosNormalTexLayout;	
	ComPtr<ID3D11InputLayout> instancePosColorLayout;
	ComPtr<ID3D11InputLayout> vertexPosNormalTexLayout;		
	ComPtr<ID3D11InputLayout> vertexPosColorLayout;

	ComPtr<ID3D11ShaderResourceView> textureA;				// 环境光对应使用的纹理
	ComPtr<ID3D11ShaderResourceView> textureD;				// 漫射光对应使用的纹理
};

//
// BasicEffect
//

namespace
{
	// BasicEffect单例
	static BasicEffect * pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
	if (pInstance)
		throw std::exception("BasicEffect is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect & BasicEffect::Get()
{
	if (!pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *pInstance;
}


bool BasicEffect::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->cBufferPtrs.empty())
		return true;


	ComPtr<ID3DBlob> blob;

	// 实例输入布局
	D3D11_INPUT_ELEMENT_DESC basicInstLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 1}
	};

	// ******************
	// 创建顶点着色器
	//

	HR(pImpl->CreateShaderFromFile(L"HLSL\\BasicInstance_VS.vso", L"HLSL\\BasicInstance_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->basicInstanceVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(basicInstLayout, ARRAYSIZE(basicInstLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->instancePosNormalTexLayout.GetAddressOf()));

	HR(pImpl->CreateShaderFromFile(L"HLSL\\BasicObject_VS.vso", L"HLSL\\BasicObject_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->basicObjectVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexPosNormalTexLayout.GetAddressOf()));

	// ******************
	// 创建像素着色器
	//

	HR(pImpl->CreateShaderFromFile(L"HLSL\\Basic_PS.pso", L"HLSL\\Basic_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->basicPS.GetAddressOf()));

	// 初始化
	RenderStates::InitAll(device);

	pImpl->cBufferPtrs.assign({
		&pImpl->cbObjDrawing,
		&pImpl->cbInstDrawing, 
		&pImpl->cbStates,
		&pImpl->cbFrame, 
		&pImpl->cbOnResize, 
		&pImpl->cbRarely});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		pBuffer->CreateBuffer(device);
	}

	return true;
}


void BasicEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext, RenderType type)
{
	if (type == RenderInstance)
	{
		deviceContext->IASetInputLayout(pImpl->instancePosNormalTexLayout.Get());
		deviceContext->VSSetShader(pImpl->basicInstanceVS.Get(), nullptr, 0);
		deviceContext->PSSetShader(pImpl->basicPS.Get(), nullptr, 0);
	}
	else
	{
		deviceContext->IASetInputLayout(pImpl->vertexPosNormalTexLayout.Get());
		deviceContext->VSSetShader(pImpl->basicObjectVS.Get(), nullptr, 0);
		deviceContext->PSSetShader(pImpl->basicPS.Get(), nullptr, 0);
	}

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);
	
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->cbObjDrawing;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixInverse(nullptr, W);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->cbOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetWorldViewProjMatrix(FXMMATRIX W, CXMMATRIX V, CXMMATRIX P)
{
	pImpl->cbObjDrawing.data.world = XMMatrixTranspose(W);
	pImpl->cbObjDrawing.data.worldInvTranspose = XMMatrixInverse(nullptr, W);
	pImpl->cbFrame.data.view = XMMatrixTranspose(V);
	pImpl->cbOnResize.data.proj = XMMatrixTranspose(P);

	auto& pCBuffers = pImpl->cBufferPtrs;
	pCBuffers[0]->isDirty = pCBuffers[2]->isDirty = pCBuffers[3]->isDirty = true;
	pImpl->isDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->cbInstDrawing;
	cBuffer.data.material = material;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTextureUsed(bool isUsed)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.textureUsed = isUsed;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTextureAmbient(ComPtr<ID3D11ShaderResourceView> texture)
{
	pImpl->textureA = texture;
}

void BasicEffect::SetTextureDiffuse(ComPtr<ID3D11ShaderResourceView> texture)
{
	pImpl->textureD = texture;
}

void XM_CALLCONV BasicEffect::SetEyePos(FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[3]->BindVS(deviceContext);
	pCBuffers[4]->BindVS(deviceContext);

	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);
	pCBuffers[3]->BindPS(deviceContext);
	pCBuffers[5]->BindPS(deviceContext);

	// 设置纹理
	if (pImpl->cbStates.data.textureUsed)
	{
		deviceContext->PSSetShaderResources(0, 1, pImpl->textureA.GetAddressOf());
		deviceContext->PSSetShaderResources(1, 1, pImpl->textureD.GetAddressOf());
	}


	if (pImpl->isDirty)
	{
		pImpl->isDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}

//
// BasicEffect::Impl实现部分
//


HRESULT BasicEffect::Impl::CreateShaderFromFile(const WCHAR * objFileNameInOut, const WCHAR * hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob ** ppBlobOut)
{
	HRESULT hr = S_OK;

	// 寻找是否有已经编译好的顶点着色器
	if (objFileNameInOut && filesystem::exists(objFileNameInOut))
	{
		HR(D3DReadFileToBlob(objFileNameInOut, ppBlobOut));
	}
	else
	{
		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		// 设置 D3DCOMPILE_DEBUG 标志用于获取着色器调试信息。该标志可以提升调试体验，
		// 但仍然允许着色器进行优化操作
		dwShaderFlags |= D3DCOMPILE_DEBUG;

		// 在Debug环境下禁用优化以避免出现一些不合理的情况
		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ComPtr<ID3DBlob> errorBlob = nullptr;
		hr = D3DCompileFromFile(hlslFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, shaderModel,
			dwShaderFlags, 0, ppBlobOut, errorBlob.GetAddressOf());
		if (FAILED(hr))
		{
			if (errorBlob != nullptr)
			{
				OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			}
			return hr;
		}

		// 若指定了输出文件名，则将着色器二进制信息输出
		if (objFileNameInOut)
		{
			HR(D3DWriteBlobToFile(*ppBlobOut, objFileNameInOut, FALSE));
		}
	}

	return hr;
}


