#include "GameApp.h"

using namespace DirectX;

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

    m_pd3dImmediateContext->CSSetShader(m_pTextureMul_CS.Get(), nullptr, 0);
    m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pTextureOutputUAV.GetAddressOf(), nullptr);
    m_pd3dImmediateContext->Dispatch(32, 32, 1);



//#if defined(DEBUG) | defined(_DEBUG)
//	graphicsAnalysis->EndCapture();
//#endif
    
    SaveDDSTextureToFile(m_pd3dImmediateContext.Get(), m_pTextureOutput.Get(), L"..\\Texture\\flareoutput.dds");
    MessageBox(nullptr, L"请打开..\\Texture文件夹观察输出文件flareoutput.dds", L"运行结束", MB_OK);
}

bool GameApp::InitResource()
{

    CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\flare.dds",
        nullptr, m_pTextureInputA.GetAddressOf());
    CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\flarealpha.dds",
        nullptr, m_pTextureInputB.GetAddressOf());

    // DXGI_FORMAT                     |  RWTexture2D<T>
    // --------------------------------+------------------
    // DXGI_FORMAT_R8G8B8A8_UNORM      |  unorm float4
    // DXGI_FORMAT_R16G16B16A16_UNORM  |  unorm float4
    // DXGI_FORMAT_R8G8B8A8_SNORM      |  snorm float4
    // DXGI_FORMAT_R16G16B16A16_SNORM  |  snorm float4
    // DXGI_FORMAT_R16G16B16A16_FLOAT  |  float4 或 half4?
    // DXGI_FORMAT_R32G32B32A32_FLOAT  |  float4
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    CD3D11_TEXTURE2D_DESC texDesc(format, 512, 512, 1, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pd3dDevice->CreateTexture2D(&texDesc, nullptr, m_pTextureOutput.GetAddressOf());
    
    // 创建无序访问视图
    CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(D3D11_UAV_DIMENSION_TEXTURE2D, format);
    m_pd3dDevice->CreateUnorderedAccessView(m_pTextureOutput.Get(), &uavDesc,
        m_pTextureOutputUAV.GetAddressOf());

    // 创建计算着色器
    ComPtr<ID3DBlob> blob;
    D3DReadFileToBlob(L"Shaders\\TextureMul_CS.cso", blob.GetAddressOf());
    m_pd3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pTextureMul_CS.GetAddressOf());

    return true;
}



