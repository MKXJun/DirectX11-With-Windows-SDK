#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;

# pragma warning(disable: 26812)

//
// BasicEffect::Impl 需要先于BasicEffect的定义
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:

	//
	// 这些结构体对应HLSL的结构体。需要按16字节对齐
	//

	struct CBChangesEveryInstanceDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
		DirectX::XMMATRIX texTransform;
	};

	struct CBChangesEveryObjectDrawing
	{
		Material material;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT3 eyePos;
		float pad;
	};

	struct CBDrawingStates
	{
		DirectX::XMVECTOR fogColor;
		int fogEnabled;
		float fogStart;
		float fogRange;
		int textureUsed;

		int wavesEnabled;
		DirectX::XMFLOAT2 displacementMapTexelSize;
		float gridSpatialStep;
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

public:
	// 必须显式指定
	Impl() : m_IsDirty() {}
	~Impl() = default;

public:
	// 需要16字节对齐的优先放在前面
	CBufferObject<0, CBChangesEveryInstanceDrawing>	m_CBInstDrawing;		// 每次实例绘制的常量缓冲区
	CBufferObject<1, CBChangesEveryObjectDrawing>	m_CBObjDrawing;		    // 每次对象绘制的常量缓冲区
	CBufferObject<2, CBChangesEveryFrame>			m_CBFrame;			    // 每帧绘制的常量缓冲区
	CBufferObject<3, CBDrawingStates>				m_CBStates;			    // 每次绘制状态改变的常量缓冲区
	CBufferObject<4, CBChangesOnResize>				m_CBOnResize;			// 每次窗口大小变更的常量缓冲区
	CBufferObject<5, CBChangesRarely>				m_CBRarely;			    // 几乎不会变更的常量缓冲区
	BOOL m_IsDirty;											                // 是否有值变更
	std::vector<CBufferBase*> m_pCBuffers;					                // 统一管理上面所有的常量缓冲区


	ComPtr<ID3D11VertexShader> m_pBasicInstanceVS;
	ComPtr<ID3D11VertexShader> m_pBasicObjectVS;

	ComPtr<ID3D11PixelShader> m_pBasicPS;

	ComPtr<ID3D11VertexShader> m_pBasicVS2D;
	ComPtr<ID3D11PixelShader> m_pBasicPS2D;

	ComPtr<ID3D11PixelShader> m_pCompositePS;

	ComPtr<ID3D11InputLayout> m_pInstancePosNormalTexLayout;
	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;		
	ComPtr<ID3D11InputLayout> m_pVertexPosTexLayout;

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;		                // 漫反射纹理
	ComPtr<ID3D11ShaderResourceView> m_pTextureDisplacement;				// 位移纹理
	ComPtr<ID3D11ShaderResourceView> m_pTextureComposite;					// 边缘纹理
};

//
// BasicEffect
//

namespace
{
	// BasicEffect单例
	static BasicEffect * g_pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
	if (g_pInstance)
		throw std::exception("BasicEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect & BasicEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *g_pInstance;
}


bool BasicEffect::InitAll(ID3D11Device * device)
{
	if (!device)
		return false;

	if (!pImpl->m_pCBuffers.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

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

	HR(CreateShaderFromFile(L"HLSL\\BasicInstance_VS.cso", L"HLSL\\BasicInstance_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicInstanceVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(basicInstLayout, ARRAYSIZE(basicInstLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pInstancePosNormalTexLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\BasicObject_VS.cso", L"HLSL\\BasicObject_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicObjectVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_2D.cso", L"HLSL\\Basic_VS_2D.hlsl", "VS_2D", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicVS2D.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosTexLayout.GetAddressOf()));

	// ******************
	// 创建像素着色器
	//

	HR(CreateShaderFromFile(L"HLSL\\Basic_PS.cso", L"HLSL\\Basic_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicPS.GetAddressOf()));


	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_2D.cso", L"HLSL\\Basic_PS_2D.hlsl", "PS_2D", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicPS2D.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Composite_PS.cso", L"HLSL\\Composite_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pCompositePS.GetAddressOf()));

	pImpl->m_pCBuffers.assign({
		&pImpl->m_CBInstDrawing,
		&pImpl->m_CBObjDrawing, 
		&pImpl->m_CBFrame,
		&pImpl->m_CBStates,
		&pImpl->m_CBOnResize, 
		&pImpl->m_CBRarely});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->m_pCBuffers)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	// 设置调试对象名
	D3D11SetDebugObjectName(pImpl->m_pInstancePosNormalTexLayout.Get(), "BasicEffect.InstancePosNormalTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "BasicEffect.VertexPosNormalTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pVertexPosTexLayout.Get(), "BasicEffect.VertexPosTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[0]->cBuffer.Get(), "BasicEffect.CBInstDrawing");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[1]->cBuffer.Get(), "BasicEffect.CBObjDrawing");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[2]->cBuffer.Get(), "BasicEffect.CBFrame");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[3]->cBuffer.Get(), "BasicEffect.CBStates");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[4]->cBuffer.Get(), "BasicEffect.CBOnResize");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[5]->cBuffer.Get(), "BasicEffect.CBRarely");
	D3D11SetDebugObjectName(pImpl->m_pBasicObjectVS.Get(), "BasicEffect.BasicObject_VS");
	D3D11SetDebugObjectName(pImpl->m_pBasicInstanceVS.Get(), "BasicEffect.BasicInstance_VS");
	D3D11SetDebugObjectName(pImpl->m_pBasicPS.Get(), "BasicEffect.Basic_PS");
	D3D11SetDebugObjectName(pImpl->m_pBasicVS2D.Get(), "BasicEffect.Basic_VS_2D");
	D3D11SetDebugObjectName(pImpl->m_pBasicPS2D.Get(), "BasicEffect.Basic_PS_2D");
	D3D11SetDebugObjectName(pImpl->m_pCompositePS.Get(), "BasicEffect.Composite_PS");
	
	return true;
}


void BasicEffect::SetRenderDefault(ID3D11DeviceContext * deviceContext, RenderType type)
{
	if (type == RenderInstance)
	{
		deviceContext->IASetInputLayout(pImpl->m_pInstancePosNormalTexLayout.Get());
		deviceContext->VSSetShader(pImpl->m_pBasicInstanceVS.Get(), nullptr, 0);
		deviceContext->PSSetShader(pImpl->m_pBasicPS.Get(), nullptr, 0);
	}
	else
	{
		deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
		deviceContext->VSSetShader(pImpl->m_pBasicObjectVS.Get(), nullptr, 0);
		deviceContext->PSSetShader(pImpl->m_pBasicPS.Get(), nullptr, 0);
	}

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	deviceContext->VSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->VSSetSamplers(1, 1, RenderStates::SSPointClamp.GetAddressOf());

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);
	
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicEffect::Set2DRenderDefault(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosTexLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pBasicVS2D.Get(), nullptr, 0);
	deviceContext->PSSetShader(pImpl->m_pBasicPS2D.Get(), nullptr, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicEffect::SetRenderComposite(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosTexLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pBasicVS2D.Get(), nullptr, 0);
	deviceContext->PSSetShader(pImpl->m_pCompositePS.Get(), nullptr, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->PSSetSamplers(1, 1, RenderStates::SSPointClamp.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->m_CBInstDrawing;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->m_CBOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetTexTransformMatrix(DirectX::FXMMATRIX T)
{
	auto& cBuffer = pImpl->m_CBInstDrawing;
	cBuffer.data.texTransform = XMMatrixTranspose(T);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->m_CBObjDrawing;
	cBuffer.data.material = material;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTextureUsed(bool isUsed)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.textureUsed = isUsed;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTextureDiffuse(ID3D11ShaderResourceView * textureDiffuse)
{
	pImpl->m_pTextureDiffuse = textureDiffuse;
}

void BasicEffect::SetTextureDisplacement(ID3D11ShaderResourceView* textureDisplacement)
{
	pImpl->m_pTextureDisplacement = textureDisplacement;
}

void BasicEffect::SetTextureComposite(ID3D11ShaderResourceView* textureComposite)
{
	pImpl->m_pTextureComposite = textureComposite;
}

void BasicEffect::SetWavesStates(bool enabled, float texelSizeU, float texelSizeV, float gridSpatialStep)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.wavesEnabled = enabled;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
	if (enabled)
	{
		cBuffer.data.displacementMapTexelSize = XMFLOAT2(texelSizeU, texelSizeV);
		cBuffer.data.gridSpatialStep = gridSpatialStep;
	}
}

void BasicEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogState(bool isOn)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogEnabled = isOn;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogStart(float fogStart)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogStart = fogStart;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogColor(DirectX::XMVECTOR fogColor)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogColor = fogColor;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogRange(float fogRange)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogRange = fogRange;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ID3D11DeviceContext * deviceContext)
{
	auto& pCBuffers = pImpl->m_pCBuffers;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[2]->BindVS(deviceContext);
	pCBuffers[3]->BindVS(deviceContext);
	pCBuffers[4]->BindVS(deviceContext);

	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);
	pCBuffers[3]->BindPS(deviceContext);
	pCBuffers[5]->BindPS(deviceContext);

	// 设置纹理
	deviceContext->VSSetShaderResources(1, 1, pImpl->m_pTextureDisplacement.GetAddressOf());
	deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTextureDiffuse.GetAddressOf());
	deviceContext->PSSetShaderResources(1, 1, pImpl->m_pTextureComposite.GetAddressOf());

	if (pImpl->m_IsDirty)
	{
		pImpl->m_IsDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}


