#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	if (!InitResource())
		return false;

	return true;
}

void GameApp::Compute()
{
	assert(m_pd3dImmediateContext);

	//#if defined(DEBUG) | defined(_DEBUG)
	//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
	//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
	//	graphicsAnalysis->BeginCapture();
	//#endif
	m_pd3dImmediateContext->CSSetShaderResources(0, 1, m_pTextureInputA.GetAddressOf());
	m_pd3dImmediateContext->CSSetShaderResources(1, 1, m_pTextureInputB.GetAddressOf());

	// DXGI Format: DXGI_FORMAT_R32G32B32A32_FLOAT
	// Pixel Format: A32B32G32R32
	m_pd3dImmediateContext->CSSetShader(m_pTextureMul_R32G32B32A32_CS.Get(), nullptr, 0);
	m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pTextureOutputA_UAV.GetAddressOf(), nullptr);
	m_pd3dImmediateContext->Dispatch(32, 32, 1);

	// DXGI Format: DXGI_FORMAT_R8G8B8A8_SNORM
	// Pixel Format: A8B8G8R8
	m_pd3dImmediateContext->CSSetShader(m_pTextureMul_R8G8B8A8_CS.Get(), nullptr, 0);
	m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pTextureOutputB_UAV.GetAddressOf(), nullptr);
	m_pd3dImmediateContext->Dispatch(32, 32, 1);


	//#if defined(DEBUG) | defined(_DEBUG)
	//	graphicsAnalysis->EndCapture();
	//#endif

	HR(SaveDDSTextureToFile(m_pd3dImmediateContext.Get(), m_pTextureOutputA.Get(), L"Texture\\flareoutputA.dds"));
	HR(SaveDDSTextureToFile(m_pd3dImmediateContext.Get(), m_pTextureOutputB.Get(), L"Texture\\flareoutputB.dds"));
	
	MessageBox(nullptr, L"请打开Texture文件夹观察输出文件flareoutputA.dds和flareoutputB.dds", L"运行结束", MB_OK);
}

bool GameApp::InitResource()
{

	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\flare.dds",
		nullptr, m_pTextureInputA.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\flarealpha.dds",
		nullptr, m_pTextureInputB.GetAddressOf()));
	
	// 创建用于UAV的纹理，必须是非压缩格式
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 512;
	texDesc.Height = 512;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE |
		D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	HR(m_pd3dDevice->CreateTexture2D(&texDesc, nullptr, m_pTextureOutputA.GetAddressOf()));
	
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	HR(m_pd3dDevice->CreateTexture2D(&texDesc, nullptr, m_pTextureOutputB.GetAddressOf()));

	// 创建无序访问视图
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	HR(m_pd3dDevice->CreateUnorderedAccessView(m_pTextureOutputA.Get(), &uavDesc,
		m_pTextureOutputA_UAV.GetAddressOf()));

	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	HR(m_pd3dDevice->CreateUnorderedAccessView(m_pTextureOutputB.Get(), &uavDesc,
		m_pTextureOutputB_UAV.GetAddressOf()));

	// 创建计算着色器
	ComPtr<ID3DBlob> blob;
	HR(CreateShaderFromFile(L"HLSL\\TextureMul_R32G32B32A32_CS.cso",
		L"HLSL\\TextureMul_R32G32B32A32_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pd3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pTextureMul_R32G32B32A32_CS.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\TextureMul_R8G8B8A8_CS.cso",
		L"HLSL\\TextureMul_R8G8B8A8_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pd3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pTextureMul_R8G8B8A8_CS.GetAddressOf()));

	UINT numElements = 60;
	D3D11_BUFFER_DESC inputDesc;
	inputDesc.Usage = D3D11_USAGE_DEFAULT;
	inputDesc.ByteWidth = sizeof(UINT) * numElements;
	inputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	inputDesc.CPUAccessFlags = 0;
	inputDesc.StructureByteStride = 0;				
	inputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	ComPtr<ID3D11Buffer> mBuffer;
	HR(m_pd3dDevice->CreateBuffer(&inputDesc, nullptr, mBuffer.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;		// 字节地址缓冲区必须使用该类型
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	srvDesc.BufferEx.FirstElement = 0;		// 起始字节偏移值
	srvDesc.BufferEx.NumElements = numElements;		// 字节数

	ComPtr<ID3D11ShaderResourceView> mBufferSRV;
	HR(m_pd3dDevice->CreateShaderResourceView(mBuffer.Get(), &srvDesc, mBufferSRV.GetAddressOf()));

	return true;
}



