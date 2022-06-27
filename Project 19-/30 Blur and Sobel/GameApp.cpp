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

    if (!m_PostProcessEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{
    D3DApp::OnResize();

    m_pDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
    m_pFLStaticNodeBuffer = std::make_unique<StructuredBuffer<FLStaticNode>>(m_pd3dDevice.Get(), m_ClientWidth * m_ClientHeight * 4);
    m_pStartOffsetBuffer = std::make_unique<ByteAddressBuffer>(m_pd3dDevice.Get(), m_ClientWidth * m_ClientHeight);
    m_pLitTexture = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, 
        DXGI_FORMAT_R8G8B8A8_UNORM, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS);
    m_pTempTexture = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, 
        DXGI_FORMAT_R8G8B8A8_UNORM, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS);

    m_pDepthTexture->SetDebugObjectName("DepthTexture");
    m_pFLStaticNodeBuffer->SetDebugObjectName("FLStaticNodeBuffer");
    m_pStartOffsetBuffer->SetDebugObjectName("StartOffsetBuffer");
    m_pLitTexture->SetDebugObjectName("LitTexture");
    m_pTempTexture->SetDebugObjectName("TempTexture");

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
    auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);
    ImGuiIO& io = ImGui::GetIO();

    // ******************
    // 第三人称摄像机的操作
    //
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        cam3rd->RotateX(io.MouseDelta.y * 0.01f);
        cam3rd->RotateY(io.MouseDelta.x * 0.01f);
    }
    cam3rd->Approach(-io.MouseWheel * 1.0f);

    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

    if (ImGui::Begin("Blur and Sobel"))
    {
        ImGui::Checkbox("Enable OIT", &m_EnabledOIT);
        if (ImGui::Checkbox("Enable Fog", &m_EnabledFog))
        {
            m_BasicEffect.SetFogState(m_EnabledFog);
        }
        static int mode = m_BlurMode;
        static const char* modeStrs[] = {
            "Sobel Mode",
            "Blur Mode"
        };
        if (ImGui::Combo("Mode", &mode, modeStrs, ARRAYSIZE(modeStrs)))
            m_BlurMode = mode;

        if (m_BlurMode)
        {
            if (ImGui::SliderInt("Blur Radius", &m_BlurRadius, 1, 15))
            {
                m_PostProcessEffect.SetBlurKernelSize(m_BlurRadius * 2 + 1);
            }
            if (ImGui::SliderFloat("Blur Sigma", &m_BlurSigma, 1.0f, 20.0f))
            {
                m_PostProcessEffect.SetBlurSigma(m_BlurSigma);
            }
            ImGui::SliderInt("Blur Times", &m_BlurTimes, 0, 5);
        }
    }

    ImGui::End();
    ImGui::Render();

    // 每1/4s生成一个随机水波
    if (m_Timer.TotalTime() - m_BaseTime >= 0.25f)
    {
        m_BaseTime += 0.25f;
        m_GpuWaves.Disturb(m_pd3dImmediateContext.Get(),
            m_RowRange(m_RandEngine), m_ColRange(m_RandEngine),
            m_MagnitudeRange(m_RandEngine));

    }

    // 更新波浪
    m_GpuWaves.Update(m_pd3dImmediateContext.Get(), dt);

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
    ID3D11RenderTargetView* pRTVs[1] = { m_EnabledOIT ? m_pTempTexture->GetRenderTarget() : m_pLitTexture->GetRenderTarget() };
    m_pd3dImmediateContext->ClearRenderTargetView(*pRTVs, gray);
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
    D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    // ******************
    // 1. 绘制不透明对象
    //
    m_BasicEffect.SetRenderDefault();
    m_Land.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // ******************
    // 2. 存放透明物体的像素片元
    //
    if (m_EnabledOIT)
    {
        m_BasicEffect.ClearOITBuffers(
            m_pd3dImmediateContext.Get(),
            m_pFLStaticNodeBuffer->GetUnorderedAccess(),
            m_pStartOffsetBuffer->GetUnorderedAccess()
        );
        m_BasicEffect.SetRenderOITStorage(
            m_pFLStaticNodeBuffer->GetUnorderedAccess(),
            m_pStartOffsetBuffer->GetUnorderedAccess(),
            m_ClientWidth);
    }
    else
    {
        m_BasicEffect.SetRenderTransparent();
    }
    m_RedBox.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_YellowBox.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_GpuWaves.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // 清空
    m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);

    // ******************
    // 3. 进行透明混合
    //
    if (m_EnabledOIT)
    {
        m_BasicEffect.RenderOIT(
            m_pd3dImmediateContext.Get(),
            m_pFLStaticNodeBuffer->GetShaderResource(),
            m_pStartOffsetBuffer->GetShaderResource(),
            m_pTempTexture->GetShaderResource(),
            m_pLitTexture->GetRenderTarget(),
            m_pCamera->GetViewPort()
        );
    }
    
    // ******************
    // 4. 滤波
    //

    // 高斯滤波
    if (m_BlurMode == 1)
    {
        for (int i = 0; i < m_BlurTimes; ++i)
        {
            m_PostProcessEffect.ComputeGaussianBlurX(m_pd3dImmediateContext.Get(), 
                m_pLitTexture->GetShaderResource(), 
                m_pTempTexture->GetUnorderedAccess(), 
                m_ClientWidth, m_ClientHeight);
            m_PostProcessEffect.ComputeGaussianBlurY(m_pd3dImmediateContext.Get(),
                m_pTempTexture->GetShaderResource(),
                m_pLitTexture->GetUnorderedAccess(),
                m_ClientWidth, m_ClientHeight);
        }

        // 直通(RGB->sRGB)
        m_PostProcessEffect.RenderComposite(m_pd3dImmediateContext.Get(),
            m_pLitTexture->GetShaderResource(),
            nullptr,
            GetBackBufferRTV(),
            m_pCamera->GetViewPort());
    }
    // Sobel滤波
    else
    {
        m_PostProcessEffect.ComputeSobel(m_pd3dImmediateContext.Get(),
            m_pLitTexture->GetShaderResource(),
            m_pTempTexture->GetUnorderedAccess(),
            m_ClientWidth, m_ClientHeight);
        m_PostProcessEffect.RenderComposite(m_pd3dImmediateContext.Get(),
            m_pLitTexture->GetShaderResource(),
            m_pTempTexture->GetShaderResource(),
            GetBackBufferRTV(),
            m_pCamera->GetViewPort());
    }

    pRTVs[0] = GetBackBufferRTV();
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
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
    // 红色盒子
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("RedBox", Geometry::CreateBox(8.0f, 8.0f, 8.0f));
        pModel->SetDebugObjectName("RedBox");
        m_TextureManager.CreateFromFile("..\\Texture\\Red.dds");
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\Red.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        pModel->materials[0].Set<float>("$Opacity", 0.5f);
        m_RedBox.SetModel(pModel);
        m_RedBox.GetTransform().SetPosition(-6.0f, 2.0f, -4.0f);
    }
    // 黄色盒子
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("YellowBox", Geometry::CreateBox(8.0f, 8.0f, 8.0f));
        pModel->SetDebugObjectName("YellowBox");
        m_TextureManager.CreateFromFile("..\\Texture\\Yellow.dds");
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\Yellow.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        pModel->materials[0].Set<float>("$Opacity", 0.5f);
        m_YellowBox.SetModel(pModel);
        m_YellowBox.GetTransform().SetPosition(-2.0f, 1.8f, 0.0f);
    }

    // ******************
    // 初始化水面波浪
    //
    m_GpuWaves.InitResource(m_pd3dDevice.Get(), 256, 256, 5.0f, 5.0f, 0.03f, 0.625f, 2.0f, 0.2f, 0.05f, 0.1f);

    // ******************
    // 初始化随机数生成器
    //
    m_RandEngine.seed(std::random_device()());
    m_RowRange = std::uniform_int_distribution<UINT>(5, m_GpuWaves.RowCount() - 5);
    m_ColRange = std::uniform_int_distribution<UINT>(5, m_GpuWaves.ColumnCount() - 5);
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

    m_PostProcessEffect.SetBlurKernelSize(m_BlurRadius * 2 + 1);
    m_PostProcessEffect.SetBlurSigma(m_BlurSigma);

    return true;
}

