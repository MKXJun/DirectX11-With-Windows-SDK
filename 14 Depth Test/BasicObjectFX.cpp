#include "Effects.h"
#include "EffectHelper.h"
#include "Vertex.h"
#include <d3dcompiler.h>
#include <experimental/filesystem>
using namespace DirectX;
using namespace std::experimental;

//
// 这些结构体对应HLSL的结构体，仅供该文件使用。需要按16字节对齐
//

struct CBChangesEveryDrawing
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX texTransform;
	Material material;
};

struct CBDrawingStates
{
	int isReflection;
	int isShadow;
	DirectX::XMINT2 pad;
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
	DirectX::XMMATRIX reflection;
	DirectX::XMMATRIX shadow;
	DirectX::XMMATRIX refShadow;
	DirectionalLight dirLight[BasicObjectFX::maxLights];
	PointLight pointLight[BasicObjectFX::maxLights];
	SpotLight spotLight[BasicObjectFX::maxLights];
};


//
// BasicObjectFX::Impl 需要先于BasicObjectFX的定义
//

class BasicObjectFX::Impl : public AlignedType<BasicObjectFX::Impl>
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
	CBufferObject<0, CBChangesEveryDrawing> cbDrawing;		// 每次对象绘制的常量缓冲区
	CBufferObject<1, CBDrawingStates>       cbStates;		// 每次绘制状态变更的常量缓冲区
	CBufferObject<2, CBChangesEveryFrame>   cbFrame;		// 每帧绘制的常量缓冲区
	CBufferObject<3, CBChangesOnResize>     cbOnResize;		// 每次窗口大小变更的常量缓冲区
	CBufferObject<4, CBChangesRarely>		cbRarely;		// 几乎不会变更的常量缓冲区
	BOOL isDirty;											// 是否有值变更
	std::vector<CBufferBase*> cBufferPtrs;					// 统一管理下面所有的常量缓冲区


	ComPtr<ID3D11VertexShader> vertexShader3D;				// 用于3D的顶点着色器
	ComPtr<ID3D11PixelShader>  pixelShader3D;				// 用于3D的像素着色器
	ComPtr<ID3D11VertexShader> vertexShader2D;				// 用于2D的顶点着色器
	ComPtr<ID3D11PixelShader>  pixelShader2D;				// 用于2D的像素着色器

	ComPtr<ID3D11InputLayout>  vertexLayout2D;				// 用于2D的顶点输入布局
	ComPtr<ID3D11InputLayout>  vertexLayout3D;				// 用于3D的顶点输入布局

	ComPtr<ID3D11ShaderResourceView> texture;				// 用于绘制的纹理

};

//
// BasicObjectFX
//

namespace
{
	// BasicObjectFX单例
	static BasicObjectFX * pInstance = nullptr;
}

BasicObjectFX::BasicObjectFX()
{
	if (pInstance)
		throw std::exception("BasicObjectFX is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<BasicObjectFX::Impl>();
}

BasicObjectFX::~BasicObjectFX()
{
}

BasicObjectFX::BasicObjectFX(BasicObjectFX && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

BasicObjectFX & BasicObjectFX::operator=(BasicObjectFX && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicObjectFX & BasicObjectFX::Get()
{
	if (!pInstance)
		throw std::exception("BasicObjectFX needs an instance!");
	return *pInstance;
}


bool BasicObjectFX::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->cBufferPtrs.empty())
		return true;


	ComPtr<ID3DBlob> blob;

	// 创建顶点着色器(2D)
	HR(pImpl->CreateShaderFromFile(L"HLSL\\BasicObject_VS_2D.vso", L"HLSL\\BasicObject_VS_2D.hlsl", "VS", "vs_5_0", blob.GetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->vertexShader2D.GetAddressOf()));
	// 创建顶点布局(2D)
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexLayout2D.GetAddressOf()));

	// 创建像素着色器(2D)
	HR(pImpl->CreateShaderFromFile(L"HLSL\\BasicObject_PS_2D.pso", L"HLSL\\BasicObject_PS_2D.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->pixelShader2D.GetAddressOf()));

	// 创建顶点着色器(3D)
	HR(pImpl->CreateShaderFromFile(L"HLSL\\BasicObject_VS_3D.vso", L"HLSL\\BasicObject_VS_3D.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->vertexShader3D.GetAddressOf()));
	// 创建顶点布局(3D)
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexLayout3D.GetAddressOf()));

	// 创建像素着色器(3D)
	HR(pImpl->CreateShaderFromFile(L"HLSL\\BasicObject_PS_3D.pso", L"HLSL\\BasicObject_PS_3D.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->pixelShader3D.GetAddressOf()));


	// 初始化
	RenderStates::InitAll(device);

	pImpl->cBufferPtrs.assign({
		&pImpl->cbDrawing, 
		&pImpl->cbFrame, 
		&pImpl->cbStates, 
		&pImpl->cbOnResize, 
		&pImpl->cbRarely});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		pBuffer->CreateBuffer(device);
	}

	return true;
}

void BasicObjectFX::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetRenderAlphaBlend(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetDrawBoltAnimNoDepthTest(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 0);
	deviceContext->OMSetBlendState(RenderStates::BSAdditive.Get(), nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetDrawBoltAnimNoDepthWrite(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);
	deviceContext->OMSetBlendState(RenderStates::BSAdditive.Get(), nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetRenderNoDoubleBlend(ComPtr<ID3D11DeviceContext> deviceContext, UINT stencilRef)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDoubleBlend.Get(), stencilRef);
	deviceContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetWriteStencilOnly(ComPtr<ID3D11DeviceContext> deviceContext, UINT stencilRef)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSWriteStencil.Get(), stencilRef);
	deviceContext->OMSetBlendState(RenderStates::BSNoColorWrite.Get(), nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetRenderDefaultWithStencil(ComPtr<ID3D11DeviceContext> deviceContext, UINT stencilRef)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSCullClockWise.Get());
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSDrawWithStencil.Get(), stencilRef);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetRenderAlphaBlendWithStencil(ComPtr<ID3D11DeviceContext> deviceContext, UINT stencilRef)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSDrawWithStencil.Get(), stencilRef);
	deviceContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetDrawBoltAnimNoDepthTestWithStencil(ComPtr<ID3D11DeviceContext> deviceContext, UINT stencilRef)
{
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDepthTestWithStencil.Get(), stencilRef);
	deviceContext->OMSetBlendState(RenderStates::BSAdditive.Get(), nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::SetDrawBoltAnimNoDepthWriteWithStencil(ComPtr<ID3D11DeviceContext> deviceContext, UINT stencilRef)
{
	deviceContext->IASetInputLayout(pImpl->vertexLayout3D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader3D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->pixelShader3D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDepthWriteWithStencil.Get(), stencilRef);
	deviceContext->OMSetBlendState(RenderStates::BSAdditive.Get(), nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::Set2DRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexLayout2D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader2D.Get(), nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->pixelShader2D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicObjectFX::Set2DRenderAlphaBlend(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexLayout2D.Get());
	deviceContext->VSSetShader(pImpl->vertexShader2D.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->pixelShader2D.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV BasicObjectFX::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->cbDrawing;
	cBuffer.data.world = W;
	cBuffer.data.worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, W));
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.view = V;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->cbOnResize;
	cBuffer.data.proj = P;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetWorldViewProjMatrix(FXMMATRIX W, CXMMATRIX V, CXMMATRIX P)
{
	pImpl->cbDrawing.data.world = W;
	pImpl->cbDrawing.data.worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, W));
	pImpl->cbFrame.data.view = V;
	pImpl->cbOnResize.data.proj = P;

	auto& pCBuffers = pImpl->cBufferPtrs;
	pCBuffers[0]->isDirty = pCBuffers[1]->isDirty = pCBuffers[3]->isDirty = true;
	pImpl->isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetTexTransformMatrix(FXMMATRIX W)
{
	auto& cBuffer = pImpl->cbDrawing;
	cBuffer.data.texTransform = W;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetReflectionMatrix(FXMMATRIX R)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.reflection = R;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetShadowMatrix(FXMMATRIX S)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.shadow = S;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetRefShadowMatrix(DirectX::FXMMATRIX RefS)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.refShadow = RefS;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->cbDrawing;
	cBuffer.data.material = material;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	pImpl->texture = texture;
}

void XM_CALLCONV BasicObjectFX::SetEyePos(FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetReflectionState(bool isOn)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.isReflection = isOn;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetShadowState(bool isOn)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.isShadow = isOn;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	pCBuffers[2]->BindVS(deviceContext);
	pCBuffers[3]->BindVS(deviceContext);
	pCBuffers[4]->BindVS(deviceContext);

	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);
	pCBuffers[4]->BindPS(deviceContext);

	// 设置纹理
	deviceContext->PSSetShaderResources(0, 1, pImpl->texture.GetAddressOf());

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
// BasicObjectFX::Impl实现部分
//


HRESULT BasicObjectFX::Impl::CreateShaderFromFile(const WCHAR * objFileNameInOut, const WCHAR * hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob ** ppBlobOut)
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


