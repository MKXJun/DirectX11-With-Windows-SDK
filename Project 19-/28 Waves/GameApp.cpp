#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;

    m_TextureManager.Init(m_pd3dDevice.Get());
    m_ModelManager.Init(m_pd3dDevice.Get());

    // 务必先初始化所有渲染状态，以供下面的特效使用
    RenderStates::InitAll(m_pd3dDevice.Get());

    if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{
    D3DApp::OnResize();

    m_pDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
    m_pLitTexture = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDepthTexture->SetDebugObjectName("DepthTexture");
    m_pLitTexture->SetDebugObjectName("LitTexture");

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_BasicEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
    }
}

void GameApp::UpdateScene(float dt)
{

    // 获取子类
    auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);

    // ******************
    // 第三人称摄像机的操作
    //

    ImGuiIO& io = ImGui::GetIO();
    // 绕物体旋转
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        cam3rd->RotateX(io.MouseDelta.y * 0.01f);
        cam3rd->RotateY(io.MouseDelta.x * 0.01f);
    }
    cam3rd->Approach(-io.MouseWheel * 1.0f);

    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

    if (ImGui::Begin("Waves"))
    {
        static const char* wavemode_strs[] = {
            "CPU",
            "GPU"
        };
        if (ImGui::Combo("Waves Mode", &m_WavesMode, wavemode_strs, ARRAYSIZE(wavemode_strs)))
        {
            if (m_WavesMode)
                m_GpuWaves.InitResource(m_pd3dDevice.Get(), 256, 256, 5.0f, 5.0f, 0.03f, 0.625f, 2.0f, 0.2f, 0.05f, 0.1f);
            else
                m_CpuWaves.InitResource(m_pd3dDevice.Get(), 256, 256, 5.0f, 5.0f, 0.03f, 0.625f, 2.0f, 0.2f, 0.05f, 0.1f);
                
        }
        if (ImGui::Checkbox("Enable Fog", &m_EnabledFog))
        {
            m_BasicEffect.SetFogState(m_EnabledFog);
        }
    }
    ImGui::End();
    ImGui::Render();

    // 每1/4s生成一个随机水波
    if (m_Timer.TotalTime() - m_BaseTime >= 0.25f)
    {
        m_BaseTime += 0.25f;
        if (m_WavesMode)
        {
            m_GpuWaves.Disturb(m_pd3dImmediateContext.Get(), 
                m_RowRange(m_RandEngine), m_ColRange(m_RandEngine),
                m_MagnitudeRange(m_RandEngine));
        }
        else
        {
            m_CpuWaves.Disturb(m_RowRange(m_RandEngine), m_ColRange(m_RandEngine),
                m_MagnitudeRange(m_RandEngine));
        }
            
    }

    // 更新波浪
    if (m_WavesMode)
        m_GpuWaves.Update(m_pd3dImmediateContext.Get(), dt);
    else
        m_CpuWaves.Update(dt);
}

void GameApp::DrawScene()
{
    // 创建后备缓冲区的渲染目标视图
    if (m_FrameCount < m_BackBufferCount)
    {
        ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, m_pRenderTargetViews[m_FrameCount].ReleaseAndGetAddressOf());
    }


    float gray[4] = { 0.75f, 0.75f, 0.75f, 1.0f };
    m_pd3dImmediateContext->ClearRenderTargetView(GetBackBufferRTV(), gray);
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ID3D11RenderTargetView* pRTVs[1] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
    D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);
    
    // ******************
    // 1. 绘制不透明对象
    //
    m_BasicEffect.SetRenderDefault();
    m_Land.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    // ******************
    // 2. 绘制半透明/透明对象
    //
    m_BasicEffect.SetRenderTransparent();
    m_WireFence.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    if (m_WavesMode)
        m_GpuWaves.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    else
        m_CpuWaves.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}



bool GameApp::InitResource()
{
    // ******************
    // 初始化游戏对象
    //
 
    // 地面
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Ground", Geometry::CreateGrid(XMFLOAT2(160.0f, 160.0f),
            XMUINT2(50, 50), XMFLOAT2(10.0f, 10.0f),
            [](float x, float z) { return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z)); },	// 高度函数
            [](float x, float z) { return XMFLOAT3{ -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z), 1.0f,
            -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z) }; }));
        pModel->SetDebugObjectName("Ground");
        m_TextureManager.CreateFromFile("..\\Texture\\grass.dds");
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\grass.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        m_Land.SetModel(pModel);
        m_Land.GetTransform().SetPosition(0.0f, -1.0f, 0.0f);
    }
    // 篱笆盒
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("WireFence", Geometry::CreateBox(8.0f, 8.0f, 8.0f));
        pModel->SetDebugObjectName("WireFence");
        m_TextureManager.CreateFromFile("..\\Texture\\WireFence.dds");
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\WireFence.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        m_WireFence.SetModel(pModel);
        m_WireFence.GetTransform().SetPosition(-2.0f, 2.0f, -4.0f);
    }
    
    // ******************
    // 初始化水面波浪
    //
    m_CpuWaves.InitResource(m_pd3dDevice.Get(), 256, 256, 5.0f, 5.0f, 0.03f, 0.625f, 2.0f, 0.2f, 0.05f, 0.1f);
    m_GpuWaves.InitResource(m_pd3dDevice.Get(), 256, 256, 5.0f, 5.0f, 0.03f, 0.625f, 2.0f, 0.2f, 0.05f, 0.1f);

    // ******************
    // 初始化随机数生成器
    //
    m_RandEngine.seed(std::random_device()());
    m_RowRange = std::uniform_int_distribution<UINT>(5, m_CpuWaves.RowCount() - 5);
    m_ColRange = std::uniform_int_distribution<UINT>(5, m_CpuWaves.ColumnCount() - 5);
    m_MagnitudeRange = std::uniform_real_distribution<float>(0.5f, 1.0f);
    
    // ******************
    // 初始化摄像机
    //
    auto camera = std::make_shared<ThirdPersonCamera>();
    m_pCamera = camera;

    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    camera->SetTarget(XMFLOAT3(0.0f, 2.5f, 0.0f));
    camera->SetDistance(20.0f);
    camera->SetDistanceMinMax(10.0f, 90.0f);
    camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    camera->SetRotationX(XM_PIDIV4);

    m_BasicEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_BasicEffect.SetProjMatrix(camera->GetProjMatrixXM());
    
    // ******************
    // 初始化不会变化的值
    //

    // 方向光
    DirectionalLight dirLight[3]{};
    dirLight[0].ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    dirLight[0].diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight[0].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
    
    dirLight[1].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    dirLight[1].diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    dirLight[1].specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
    dirLight[1].direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
    
    dirLight[2].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    dirLight[2].diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    dirLight[2].specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    dirLight[2].direction = XMFLOAT3(0.0f, -0.707f, -0.707f);
    for (int i = 0; i < 3; ++i)
        m_BasicEffect.SetDirLight(i, dirLight[i]);

    // ******************
    // 初始化雾效和绘制状态
    //

    m_BasicEffect.SetFogState(true);
    m_BasicEffect.SetFogColor(XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f));
    m_BasicEffect.SetFogStart(15.0f);
    m_BasicEffect.SetFogRange(135.0f);

    return true;
}

