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

    if (!m_SkyboxEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_ShadowEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_SSAOEffect.InitAll(m_pd3dDevice.Get()))
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
    m_pDebugAOTexture = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth / 2, m_ClientHeight / 2, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_SSAOManager.OnResize(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);

    m_pDepthTexture->SetDebugObjectName("DepthTexture");
    m_pLitTexture->SetDebugObjectName("LitTexture");
    m_pDebugAOTexture->SetDebugObjectName("DebugAOTexture");

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_BasicEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_SSAOEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
    }
}

void GameApp::UpdateScene(float dt)
{
    m_CameraController.Update(dt);

    if (ImGui::Begin("SSAO"))
    {
        ImGui::Checkbox("Animate Light", &m_UpdateLight);
        ImGui::Checkbox("Enable Normal map", &m_EnableNormalMap);
        ImGui::Separator();
        if (ImGui::Checkbox("Enable SSAO", &m_EnableSSAO))
        {
            if (!m_EnableSSAO)
                m_EnableDebug = false;
            m_BasicEffect.SetSSAOEnabled(m_EnableSSAO);
        }
        if (m_EnableSSAO)
        {
            ImGui::SliderFloat("Epsilon", &m_SSAOManager.m_SurfaceEpsilon, 0.0f, 0.1f, "%.2f");
            static float range = m_SSAOManager.m_OcclusionFadeEnd - m_SSAOManager.m_OcclusionFadeStart;
            ImGui::SliderFloat("Fade Start", &m_SSAOManager.m_OcclusionFadeStart, 0.0f, 2.0f, "%.2f");
            if (ImGui::SliderFloat("Fade Range", &range, 0.0f, 3.0f, "%.2f"))
            {
                m_SSAOManager.m_OcclusionFadeEnd = m_SSAOManager.m_OcclusionFadeStart + range;
            }
            ImGui::SliderFloat("Sample Radius", &m_SSAOManager.m_OcclusionRadius, 0.0f, 2.0f, "%.1f");
            ImGui::SliderInt("Sample Count", reinterpret_cast<int*>(&m_SSAOManager.m_SampleCount), 1, 14);
            ImGui::Checkbox("Debug SSAO", &m_EnableDebug);
            
        }
        ImGui::Separator();
    }
    ImGui::End();

    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());
    m_SSAOEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_SkyboxEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());

    // 更新光照
    static float theta = 0;	
    if (m_UpdateLight)
    {
        theta += dt * XM_2PI / 40.0f;
    }

    for (int i = 0; i < 3; ++i)
    {
        XMVECTOR dirVec = XMLoadFloat3(&m_OriginalLightDirs[i]);
        dirVec = XMVector3Transform(dirVec, XMMatrixRotationY(theta));
        XMStoreFloat3(&m_DirLights[i].direction, dirVec);
        m_BasicEffect.SetDirLight(i, m_DirLights[i]);
    }

    //
    // 投影区域为正方体，以原点为中心，以方向光为+Z朝向
    //
    XMVECTOR dirVec = XMLoadFloat3(&m_DirLights[0].direction);
    XMMATRIX LightView = XMMatrixLookAtLH(dirVec * 20.0f * (-2.0f), g_XMZero, g_XMIdentityR1);
    m_ShadowEffect.SetViewMatrix(LightView);
    
    // 将NDC空间 [-1, +1]^2 变换到纹理坐标空间 [0, 1]^2
    static XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);
    // S = V * P * T
    m_BasicEffect.SetShadowTransformMatrix(LightView * XMMatrixOrthographicLH(40.0f, 40.0f, 20.0f, 60.0f) * T);

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
    
    if (m_EnableSSAO)
    {
        RenderSSAO();
    }
    RenderShadow();
    RenderForward();
    RenderSkybox();

    if (m_EnableDebug)
    {
        if (ImGui::Begin("SSAO Buffer", &m_EnableDebug))
        {
            CD3D11_VIEWPORT vp(0.0f, 0.0f, (float)m_pDebugAOTexture->GetWidth(), (float)m_pDebugAOTexture->GetHeight());
            m_SSAOEffect.RenderAmbientOcclusionToTexture(
                m_pd3dImmediateContext.Get(),
                m_SSAOManager.GetAmbientOcclusionTexture(),
                m_pDebugAOTexture->GetRenderTarget(),
                vp);

            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
            ImGui::Image(m_pDebugAOTexture->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
        }
        ImGui::End();
    }
    ImGui::Render();
    
    ID3D11RenderTargetView* pRTVs[]{ GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}

void GameApp::RenderSSAO()
{
    // Pass 1: 绘制场景
    m_SSAOManager.Begin(m_pd3dImmediateContext.Get(), m_pDepthTexture->GetDepthStencil(), m_pCamera->GetViewPort());
    {
        m_SSAOEffect.SetRenderNormalDepthMap(m_pd3dImmediateContext.Get());
        DrawScene<SSAOEffect>(m_SSAOEffect);
    }
    m_SSAOManager.End(m_pd3dImmediateContext.Get());

    // Pass 2: 生成AO
    m_SSAOManager.RenderToSSAOTexture(m_pd3dImmediateContext.Get(), m_SSAOEffect, *m_pCamera);

    // Pass 3: 混合
    m_SSAOManager.BlurAmbientMap(m_pd3dImmediateContext.Get(), m_SSAOEffect);

}

void GameApp::RenderShadow()
{
    CD3D11_VIEWPORT shadowViewport(0.0f, 0.0f, (float)m_pShadowMapTexture->GetWidth(), (float)m_pShadowMapTexture->GetHeight());
    m_pd3dImmediateContext->ClearDepthStencilView(m_pShadowMapTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, m_pShadowMapTexture->GetDepthStencil());
    m_pd3dImmediateContext->RSSetViewports(1, &shadowViewport);

    m_ShadowEffect.SetRenderDepthOnly();
    DrawScene<ShadowEffect>(m_ShadowEffect);
}

void GameApp::RenderForward()
{
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ID3D11RenderTargetView* pRTVs[]{ m_pLitTexture->GetRenderTarget() };
    m_pd3dImmediateContext->ClearRenderTargetView(pRTVs[0], black);
    // 开启SSAO时不要清空深度缓冲区，因为要使用相等比较
    if (!m_EnableSSAO)
    {
        m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
    D3D11_VIEWPORT vp = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &vp);

    m_BasicEffect.SetTextureShadowMap(m_pShadowMapTexture->GetShaderResource());
    m_BasicEffect.SetTextureAmbientOcclusion(m_EnableSSAO ? m_SSAOManager.GetAmbientOcclusionTexture() : nullptr);

    if (m_EnableNormalMap)
        m_BasicEffect.SetRenderWithNormalMap();
    else
        m_BasicEffect.SetRenderDefault();
    DrawScene<BasicEffect>(m_BasicEffect, [](BasicEffect& effect, ID3D11DeviceContext* deviceContext) {
        effect.SetRenderDefault();
        });

    m_BasicEffect.SetTextureShadowMap(nullptr);
    m_BasicEffect.SetTextureAmbientOcclusion(nullptr);
    m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
}

void GameApp::RenderSkybox()
{
    D3D11_VIEWPORT skyboxViewport = m_pCamera->GetViewPort();
    skyboxViewport.MinDepth = 1.0f;
    skyboxViewport.MaxDepth = 1.0f;
    m_pd3dImmediateContext->RSSetViewports(1, &skyboxViewport);

    m_SkyboxEffect.SetRenderDefault();
    m_SkyboxEffect.SetDepthTexture(m_pDepthTexture->GetShaderResource());
    m_SkyboxEffect.SetLitTexture(m_pLitTexture->GetShaderResource());

    // 由于全屏绘制，不需要用到深度缓冲区，也就不需要清空后备缓冲区了
    ID3D11RenderTargetView* pRTVs[] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
    m_Skybox.Draw(m_pd3dImmediateContext.Get(), m_SkyboxEffect);

    // 解除绑定
    m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_SkyboxEffect.SetDepthTexture(nullptr);
    m_SkyboxEffect.SetLitTexture(nullptr);
    m_SkyboxEffect.Apply(m_pd3dImmediateContext.Get());
}

bool GameApp::InitResource()
{
    // ******************
    // 初始化摄像机
    //

    auto camera = std::make_shared<FirstPersonCamera>();
    m_pCamera = camera;

    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    camera->LookTo(XMFLOAT3(0.0f, 0.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
    m_CameraController.InitCamera(m_pCamera.get());

    // ******************
    // 初始化阴影贴图和特效
    //
    m_pShadowMapTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), 2048, 2048);

    m_pShadowMapTexture->SetDebugObjectName("ShadowMapTexture");

    m_BasicEffect.SetSSAOEnabled(m_EnableSSAO);
    m_BasicEffect.SetDepthBias(0.005f);
    m_BasicEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_BasicEffect.SetProjMatrix(camera->GetProjMatrixXM());

    m_SSAOEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_SSAOEffect.SetProjMatrix(camera->GetProjMatrixXM());

    m_ShadowEffect.SetProjMatrix(XMMatrixOrthographicLH(40.0f, 40.0f, 20.0f, 60.0f));

    m_SkyboxEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_SkyboxEffect.SetProjMatrix(camera->GetProjMatrixXM());

    // ******************
    // 初始化SSAO管理
    //
    m_SSAOManager.InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);

    // ******************
    // 初始化对象
    //

    // 地面
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Ground", Geometry::CreatePlane(XMFLOAT2(20.0f, 30.0f), XMFLOAT2(6.0f, 9.0f)));
        pModel->SetDebugObjectName("Ground");
        m_TextureManager.CreateFromFile("..\\Texture\\floor.dds", false, true);
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\floor.dds");
        m_TextureManager.CreateFromFile("..\\Texture\\floor_nmap.dds");
        pModel->materials[0].Set<std::string>("$Normal", "..\\Texture\\floor_nmap.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        m_Ground.SetModel(pModel);
        m_Ground.GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
    }

    // 圆柱
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Cylinder", Geometry::CreateCylinder(0.5f, 3.0f));
        pModel->SetDebugObjectName("Cylinder");
        m_TextureManager.CreateFromFile("..\\Texture\\bricks.dds", false, true);
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\bricks.dds");
        m_TextureManager.CreateFromFile("..\\Texture\\bricks_nmap.dds");
        pModel->materials[0].Set<std::string>("$Normal", "..\\Texture\\bricks_nmap.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);

        for (size_t i = 0; i < 10; ++i)
        {
            m_Cylinders[i].SetModel(pModel);
            m_Cylinders[i].GetTransform().SetPosition(-6.0f + 12.0f * (i / 5), -1.5f, -10.0f + (i % 5) * 5.0f);
        }
    }

    // 石球
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Sphere", Geometry::CreateSphere(0.5f));
        pModel->SetDebugObjectName("Sphere");
        m_TextureManager.CreateFromFile("..\\Texture\\stone.dds", false, true);
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\stone.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);

        for (size_t i = 0; i < 10; ++i)
        {
            m_Spheres[i].SetModel(pModel);
            m_Spheres[i].GetTransform().SetPosition(-6.0f + 12.0f * (i / 5), 0.5f, -10.0f + (i % 5) * 5.0f);
        }
    }

    // 房屋
    {
        Model* pModel = m_ModelManager.CreateFromFile("..\\Model\\house.obj");
        pModel->SetDebugObjectName("House");
        m_House.SetModel(pModel);

        XMMATRIX S = XMMatrixScaling(0.01f, 0.01f, 0.01f);
        BoundingBox houseBox = m_House.GetBoundingBox();
        houseBox.Transform(houseBox, S);

        Transform& houseTransform = m_House.GetTransform();
        houseTransform.SetScale(0.01f, 0.01f, 0.01f);
        houseTransform.SetPosition(0.0f, -(houseBox.Center.y - houseBox.Extents.y + 3.0f), 0.0f);
    }

    // 天空盒
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Skybox", Geometry::CreateBox());
        pModel->SetDebugObjectName("Skybox");
        m_Skybox.SetModel(pModel);
        m_TextureManager.CreateFromFile("..\\Texture\\desertcube1024.dds", false, true);
        pModel->materials[0].Set<std::string>("$Skybox", "..\\Texture\\desertcube1024.dds");
    }

    // ******************
    // 初始化光照
    //
    m_DirLights[0].ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    m_DirLights[0].diffuse = XMFLOAT4(0.7f, 0.7f, 0.6f, 1.0f);
    m_DirLights[0].specular = XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
    m_DirLights[0].direction = XMFLOAT3(5.0f / sqrtf(50.0f), -5.0f / sqrtf(50.0f), 0.0f);

    m_DirLights[1].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    m_DirLights[1].diffuse = XMFLOAT4(0.40f, 0.40f, 0.40f, 1.0f);
    m_DirLights[1].specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    m_DirLights[1].direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

    m_DirLights[2].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    m_DirLights[2].diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    m_DirLights[2].specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    m_DirLights[2].direction = XMFLOAT3(0.0f, 0.0, -1.0f);

    for (int i = 0; i < 3; ++i)
    {
        m_OriginalLightDirs[i] = m_DirLights[i].direction;
        m_BasicEffect.SetDirLight(i, m_DirLights[i]);
    }

    return true;
}

