#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;


//
// BasicEffect::Impl 需要先于BasicEffect的定义
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:

	//
	// 这些结构体对应HLSL的结构体。需要按16字节对齐
	//

	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
		Material material;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX viewProj;
		DirectX::XMFLOAT3 eyePos;
		float pad;
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
	XMMATRIX m_World{}, m_View{}, m_Proj{};
	CBufferObject<0, CBChangesEveryDrawing> m_CBDrawing;		// 每次对象绘制的常量缓冲区
	CBufferObject<1, CBChangesEveryFrame>   m_CBFrame;		    // 每帧绘制的常量缓冲区
	CBufferObject<2, CBChangesRarely>		m_CBRarely;		    // 几乎不会变更的常量缓冲区
	BOOL m_IsDirty;											    // 是否有值变更
	std::vector<CBufferBase*> m_pCBuffers;					    // 统一管理上面所有的常量缓冲区

	

	ComPtr<ID3D11VertexShader> m_pBasicVS;
	ComPtr<ID3D11PixelShader> m_pBasicPS;

	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;		// 3D顶点输入布局

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;		    // 漫反射紋理
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


	// 创建顶点着色器
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS.cso", L"HLSL\\Basic_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

	// 创建像素着色器
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS.cso", L"HLSL\\Basic_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicPS.GetAddressOf()));


	pImpl->m_pCBuffers.assign({
		&pImpl->m_CBDrawing, 
		&pImpl->m_CBFrame, 
		&pImpl->m_CBRarely});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->m_pCBuffers)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	// 设置调试对象名
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "VertexPosNormalTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[0]->cBuffer.Get(), "CBDrawing");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[1]->cBuffer.Get(), "CBFrame");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[2]->cBuffer.Get(), "CBRarely");
	D3D11SetDebugObjectName(pImpl->m_pBasicVS.Get(), "Basic_VS");
	D3D11SetDebugObjectName(pImpl->m_pBasicPS.Get(), "Basic_PS");

	return true;
}

void BasicEffect::SetRenderDefault(ID3D11DeviceContext * deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pBasicVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->m_pBasicPS.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->m_CBDrawing;
	pImpl->m_World = W;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->m_CBFrame;
	pImpl->m_View = V;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->m_CBFrame;
	pImpl->m_Proj = P;
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
	auto& cBuffer = pImpl->m_CBDrawing;
	cBuffer.data.material = material;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTextureDiffuse(ID3D11ShaderResourceView * textureDiffuse)
{
	pImpl->m_pTextureDiffuse = textureDiffuse;
}

void BasicEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ID3D11DeviceContext * deviceContext)
{
	pImpl->m_CBDrawing.data.world = XMMatrixTranspose(pImpl->m_World);
	pImpl->m_CBDrawing.data.worldInvTranspose = XMMatrixTranspose(InverseTranspose(pImpl->m_World));
	pImpl->m_CBFrame.data.viewProj = XMMatrixTranspose(pImpl->m_View * pImpl->m_Proj);

	auto& pCBuffers = pImpl->m_pCBuffers;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);

	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);

	// 设置纹理
	deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTextureDiffuse.GetAddressOf());

	if (pImpl->m_IsDirty)
	{
		pImpl->m_IsDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}
