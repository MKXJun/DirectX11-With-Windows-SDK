#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>
using namespace DirectX;

#pragma warning(disable: 26812)

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight)
{
}

GameApp::~GameApp()
{
    /*ComPtr<ID3D11Debug> pDebug;
    m_pd3dDevice.As(&pDebug);
    pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);*/
}

bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;

    m_TextureManager.Init(m_pd3dDevice.Get());
    m_ModelManager.Init(m_pd3dDevice.Get());

    m_GpuTimer_Shadow.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    m_GpuTimer_Lighting.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    m_GpuTimer_Skybox.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    m_GpuTimer_PostProcess.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());

    // 务必先初始化所有渲染状态，以供下面的特效使用
    RenderStates::InitAll(m_pd3dDevice.Get());

    if (!m_ForwardEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_ShadowEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_SkyboxEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_FXAAEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{
    D3DApp::OnResize();

    DXGI_SAMPLE_DESC sampleDesc{};
    sampleDesc.Count = m_MsaaSamples;
    sampleDesc.Quality = 0;
    m_pLitBuffer = std::make_unique<Texture2DMS>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, sampleDesc);
    m_pDepthBuffer = std::make_unique<Depth2DMS>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, sampleDesc, DepthStencilBitsFlag::Depth_32Bits);
    m_pTempBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

    // 摄像机变更显示
    if (m_pViewerCamera != nullptr)
    {
        m_pViewerCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 300.0f);
        m_pViewerCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        // 注意：反向Z
        m_ForwardEffect.SetProjMatrix(m_pViewerCamera->GetProjMatrixXM(true));
    }
}

void GameApp::UpdateScene(float dt)
{
    // 更新摄像机
    if (m_CSManager.m_SelectedCamera <= CameraSelection::CameraSelection_Light)
        m_FPSCameraController.Update(dt);

#pragma region IMGUI
    bool need_gpu_timer_reset = false;
    if (ImGui::Begin("FXAA"))
    {
        if (ImGui::Checkbox("Enable FXAA", &m_EnableFXAA))
        {
            need_gpu_timer_reset = true;
        }

        float windowWidth = ImGui::GetWindowWidth();
        ImGui::PushItemWidth(std::max(windowWidth - 160, 10.0f));
        
        if (m_EnableFXAA)
        {
            if (ImGui::Checkbox("Enable FXAA Debug", &m_DebugFXAA))
            {
                m_FXAAEffect.EnableDebug(m_DebugFXAA);
            }

            static int majorQuality = 3;
            static int minorQuality = 9;
            if (ImGui::SliderInt("Major Quality", &majorQuality, 1, 3))
            {
                need_gpu_timer_reset = true;
                switch (majorQuality)
                {
                case 1: minorQuality = std::clamp(minorQuality, 0, 5); break;
                case 2: minorQuality = std::clamp(minorQuality, 0, 9); break;
                case 3: minorQuality = 9; break;
                default:
                    break;
                }
                m_FXAAEffect.SetQuality(majorQuality, minorQuality);
            }
            if (majorQuality < 3 && ImGui::SliderInt("Minor Quality", &minorQuality, 0, majorQuality == 1 ? 5 : 9))
            {
                need_gpu_timer_reset = true;
                m_FXAAEffect.SetQuality(majorQuality, minorQuality);
            }

            static int subpixLevel = 3;
            if (ImGui::SliderInt("SubPixel:", &subpixLevel, 0, 4, ""))
            {
                m_FXAAEffect.SetQualitySubPix(0.25f * subpixLevel);
            }
            ImGui::SameLine();
            ImGui::Text("%.2f", 0.25f * subpixLevel);

            static int thresholdLevel = 2;
            float thresholdLevelArray[] = { 0.063f, 0.125f, 0.166f, 0.250f, 0.333f };
            if (ImGui::SliderInt("Threshold:", &thresholdLevel, 0, 4, ""))
            {
                need_gpu_timer_reset = true;
                m_FXAAEffect.SetQualityEdgeThreshold(thresholdLevelArray[thresholdLevel]);
            }
            ImGui::SameLine();
            ImGui::Text("%.3f", thresholdLevelArray[thresholdLevel]);

            static int thresholdMinLevel = 0;
            float thresholdMinLevelArray[] = { 0.0833f, 0.0625f, 0.0312f };
            if (ImGui::SliderInt("ThresholdMin:", &thresholdMinLevel, 0, 2, ""))
            {
                need_gpu_timer_reset = true;
                m_FXAAEffect.SetQualityEdgeThresholdMin(thresholdMinLevelArray[thresholdMinLevel]);
            }
            ImGui::SameLine();
            ImGui::Text("%.4f", thresholdMinLevelArray[thresholdMinLevel]);               
        }

        static const char* msaa_mode_strs[] = {
            "None",
            "2x MSAA",
            "4x MSAA",
            "8x MSAA"
        };
        static int curr_scene_msaa_item = 0;
        if (ImGui::Combo("Scene MSAA", &curr_scene_msaa_item, msaa_mode_strs, ARRAYSIZE(msaa_mode_strs)))
        {
            m_MsaaSamples = (1 << curr_scene_msaa_item);
            
            DXGI_SAMPLE_DESC sampleDesc;
            sampleDesc.Count = m_MsaaSamples;
            sampleDesc.Quality = 0;
            m_pLitBuffer = std::make_unique<Texture2DMS>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, sampleDesc);
            m_pDepthBuffer = std::make_unique<Depth2DMS>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, sampleDesc, DepthStencilBitsFlag::Depth_32Bits);
            m_pTempBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
            m_SkyboxEffect.SetMsaaSamples(m_MsaaSamples);
            need_gpu_timer_reset = true;
        }

        ImGui::PopItemWidth();
    }
    ImGui::End();

    if (need_gpu_timer_reset)
    {
        m_GpuTimer_Lighting.Reset(m_pd3dImmediateContext.Get());
        m_GpuTimer_Shadow.Reset(m_pd3dImmediateContext.Get());
        m_GpuTimer_Skybox.Reset(m_pd3dImmediateContext.Get());
        m_GpuTimer_PostProcess.Reset(m_pd3dImmediateContext.Get());
    }
#pragma endregion

    if (m_CSManager.m_SelectedCamera == CameraSelection::CameraSelection_Eye)
    {
        // 注意：反向Z
        m_ForwardEffect.SetViewMatrix(m_pViewerCamera->GetViewMatrixXM());
        m_ForwardEffect.SetProjMatrix(m_pViewerCamera->GetProjMatrixXM(true));
        m_SkyboxEffect.SetViewMatrix(m_pViewerCamera->GetViewMatrixXM());
        m_SkyboxEffect.SetProjMatrix(m_pViewerCamera->GetProjMatrixXM(true));
    }
    else if (m_CSManager.m_SelectedCamera == CameraSelection::CameraSelection_Light)
    {
        // 注意：反向Z
        m_ForwardEffect.SetViewMatrix(m_pLightCamera->GetViewMatrixXM());
        m_ForwardEffect.SetProjMatrix(m_pLightCamera->GetProjMatrixXM(true));
        m_SkyboxEffect.SetViewMatrix(m_pLightCamera->GetViewMatrixXM());
        m_SkyboxEffect.SetProjMatrix(m_pLightCamera->GetProjMatrixXM(true));
    }
    else
    {
        // 注意：反向Z
        XMMATRIX ShadowProjRZ = m_CSManager.GetShadowProjectionXM(
            static_cast<int>(m_CSManager.m_SelectedCamera) - 2);
        ShadowProjRZ.r[2] *= g_XMNegateZ.v;
        ShadowProjRZ.r[3] = XMVectorSetZ(ShadowProjRZ.r[3], 1.0f - XMVectorGetZ(ShadowProjRZ.r[3]));
        
        m_ForwardEffect.SetViewMatrix(m_pLightCamera->GetViewMatrixXM());
        m_ForwardEffect.SetProjMatrix(ShadowProjRZ);
        m_SkyboxEffect.SetViewMatrix(m_pLightCamera->GetViewMatrixXM());
        m_SkyboxEffect.SetProjMatrix(ShadowProjRZ);
    }
        
    m_ShadowEffect.SetViewMatrix(m_pLightCamera->GetViewMatrixXM());

    m_CSManager.UpdateFrame(*m_pViewerCamera, *m_pLightCamera, m_Powerplant.GetModel()->boundingbox);
    m_ForwardEffect.SetLightDir(m_pLightCamera->GetLookAxis());
}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    // 创建后备缓冲区的渲染目标视图
    if (m_FrameCount < m_BackBufferCount)
    {
        ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, m_pRenderTargetViews[m_FrameCount].ReleaseAndGetAddressOf());
    }

    //
    // 场景渲染部分
    //
    RenderShadowForAllCascades();
    RenderForward();
    RenderSkybox();
    PostProcess();

    //
    // ImGui部分
    //
    if (ImGui::Begin("FXAA"))
    {
        ImGui::Separator();
        ImGui::Text("GPU Profile");
        double total_time = 0.0f;

        m_GpuTimer_Shadow.TryGetTime(nullptr);
        ImGui::Text("Shadow Pass: %.3f ms", m_GpuTimer_Shadow.AverageTime() * 1000);
        total_time += m_GpuTimer_Shadow.AverageTime();

        m_GpuTimer_Lighting.TryGetTime(nullptr);
        ImGui::Text("Lighting Pass: %.3f ms", m_GpuTimer_Lighting.AverageTime() * 1000);
        total_time += m_GpuTimer_Lighting.AverageTime();

        m_GpuTimer_Skybox.TryGetTime(nullptr);
        ImGui::Text("Skybox Pass: %.3f ms", m_GpuTimer_Skybox.AverageTime() * 1000);
        total_time += m_GpuTimer_Skybox.AverageTime();

        if (m_EnableFXAA)
        {
            m_GpuTimer_PostProcess.TryGetTime(nullptr);
            ImGui::Text("PostProcess Pass: %.3f ms", m_GpuTimer_PostProcess.AverageTime() * 1000);
            total_time += m_GpuTimer_PostProcess.AverageTime();
        }

        ImGui::Text("Total: %.3f ms", total_time * 1000);
    }
    ImGui::End();

    D3D11_VIEWPORT vp = m_pViewerCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &vp);
    ImGui::Render();

    ID3D11RenderTargetView* pRTVs[] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}
bool GameApp::InitResource()
{
    // ******************
    // 初始化摄像机和控制器
    //
    D3D11_FEATURE_DATA_FORMAT_SUPPORT2 formatSupport2;
    formatSupport2.InFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    formatSupport2.OutFormatSupport2 = D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD;
    D3D11_FEATURE_DATA_D3D11_OPTIONS options{};
    // options.ClearView
    HRESULT hr = m_pd3dDevice->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &formatSupport2, sizeof (D3D11_FEATURE_DATA_FORMAT_SUPPORT2));
    
    auto viewerCamera = std::make_shared<FirstPersonCamera>();
    m_pViewerCamera = viewerCamera;

    m_pViewerCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    m_pViewerCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 300.0f);
    viewerCamera->LookAt(XMFLOAT3(100.0f, 5.0f, 5.0f), XMFLOAT3(), XMFLOAT3(0.0f, 1.0f, 0.0f));

    auto lightCamera = std::make_shared<FirstPersonCamera>();
    m_pLightCamera = lightCamera;

    m_pLightCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    lightCamera->LookAt(XMFLOAT3(-320.0f, 300.0f, -220.3f), XMFLOAT3(), XMFLOAT3(0.0f, 1.0f, 0.0f));
    lightCamera->SetFrustum(XM_PI / 3, 1.0f, 0.1f, 1000.0f);

    m_FPSCameraController.InitCamera(viewerCamera.get());
    m_FPSCameraController.SetMoveSpeed(10.0f);
    // ******************
    // 初始化特效
    //

    m_ForwardEffect.SetViewMatrix(viewerCamera->GetViewMatrixXM());
    m_ForwardEffect.SetProjMatrix(viewerCamera->GetProjMatrixXM(true));

    m_ForwardEffect.SetShadowType(static_cast<int>(m_CSManager.m_ShadowType));
    m_ForwardEffect.SetShadowSize(m_CSManager.m_ShadowSize);
    m_ForwardEffect.SetCascadeBlendArea(m_CSManager.m_BlendBetweenCascadesRange);
    m_ForwardEffect.SetCascadeLevels(m_CSManager.m_CascadeLevels);
    m_ForwardEffect.SetCascadeIntervalSelectionEnabled(static_cast<bool>(m_CSManager.m_SelectedCascadeSelection));
    m_ForwardEffect.SetPCFKernelSize(m_CSManager.m_BlurKernelSize);
    m_ForwardEffect.SetPCFDepthBias(m_CSManager.m_PCFDepthBias);
    m_ForwardEffect.SetLightBleedingReduction(m_CSManager.m_LightBleedingReduction);
    m_ForwardEffect.SetMagicPower(m_CSManager.m_MagicPower);
    m_ForwardEffect.SetPosExponent(m_CSManager.m_PosExp);
    m_ForwardEffect.SetNegExponent(m_CSManager.m_NegExp);
    m_ForwardEffect.Set16BitFormatShadow(false);

    m_ShadowEffect.SetViewMatrix(lightCamera->GetViewMatrixXM());
    m_ShadowEffect.SetBlurKernelSize(m_CSManager.m_BlurKernelSize);
    m_ShadowEffect.SetBlurSigma(m_CSManager.m_GaussianBlurSigma);
    m_ShadowEffect.Set16BitFormatShadow(false);

    m_SkyboxEffect.SetMsaaSamples(1);

    m_FXAAEffect.SetQualitySubPix(0.75f);
    m_FXAAEffect.SetQualityEdgeThreshold(0.166f);
    m_FXAAEffect.SetQualityEdgeThresholdMin(0.0833f);
    m_FXAAEffect.SetQuality(3, 9);
    

    // ******************
    // 初始化对象
    //
    m_Powerplant.SetModel(m_ModelManager.CreateFromFile("..\\Model\\powerplant\\powerplant.gltf"));

    m_ModelManager.CreateFromGeometry("cube", Geometry::CreateBox());
    m_Cube.SetModel(m_ModelManager.GetModel("cube"));
    m_Cube.GetTransform().SetPosition(XMFLOAT3(48.0f, 1.0f, 0.0f));

    m_TextureManager.CreateFromFile("..\\Texture\\Clouds.dds");
    m_ModelManager.CreateFromGeometry("skyboxCube", Geometry::CreateBox());
    Model* pModel = m_ModelManager.GetModel("skyboxCube");
    pModel->materials[0].Set<std::string>("$Skybox", "..\\Texture\\Clouds.dds");
    m_Skybox.SetModel(pModel);

    // ******************
    // 初始化阴影
    //
    m_CSManager.InitResource(m_pd3dDevice.Get());
    m_pDebugShadowBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_CSManager.m_ShadowSize, m_CSManager.m_ShadowSize, DXGI_FORMAT_R8G8B8A8_UNORM);
    
    // ******************
    // 设置调试对象名
    //
    m_pLitBuffer->SetDebugObjectName("LitBuffer");

    return true;
}

void GameApp::RenderShadowForAllCascades()
{
    m_GpuTimer_Shadow.Start();
    {
        //
        // 绘制深度图
        //
        
        D3D11_VIEWPORT vp = m_CSManager.GetShadowViewport();
        m_pd3dImmediateContext->RSSetViewports(1, &vp);
        float clearColor[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
        for (size_t cascadeIdx = 0; cascadeIdx < m_CSManager.m_CascadeLevels; ++cascadeIdx)
        {
            m_ShadowEffect.SetRenderDefault();

            ID3D11DepthStencilView* depthDSV = m_CSManager.GetDepthBufferDSV();
            ID3D11RenderTargetView* depthRTV = m_CSManager.GetCascadeRenderTargetView(cascadeIdx);
            m_pd3dImmediateContext->ClearRenderTargetView(depthRTV, clearColor);
            m_pd3dImmediateContext->ClearDepthStencilView(depthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
            if (m_CSManager.m_ShadowType != ShadowType::ShadowType_CSM)
                depthRTV = nullptr;
            m_pd3dImmediateContext->OMSetRenderTargets(1, &depthRTV, depthDSV);

            XMMATRIX shadowProj = m_CSManager.GetShadowProjectionXM(cascadeIdx);
            m_ShadowEffect.SetProjMatrix(shadowProj);

            // 更新物体与投影立方体的裁剪
            BoundingOrientedBox obb = m_CSManager.GetShadowOBB(cascadeIdx);
            obb.Transform(obb, m_pLightCamera->GetLocalToWorldMatrixXM());
            m_Powerplant.CubeCulling(obb);

            m_Powerplant.Draw(m_pd3dImmediateContext.Get(), m_ShadowEffect);
            
            m_Cube.CubeCulling(obb);
            m_Cube.Draw(m_pd3dImmediateContext.Get(), m_ShadowEffect);

            m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
        }
    }

    m_GpuTimer_Shadow.Stop();
}

void GameApp::RenderForward()
{
    m_GpuTimer_Lighting.Start();
    {
        float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_pd3dImmediateContext->ClearRenderTargetView(m_pLitBuffer->GetRenderTarget(), black);
        // 注意：反向Z
        m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthBuffer->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

        D3D11_VIEWPORT viewport = m_pViewerCamera->GetViewPort();
        BoundingFrustum frustum;
        BoundingFrustum::CreateFromMatrix(frustum, m_pViewerCamera->GetProjMatrixXM());
        frustum.Transform(frustum, m_pViewerCamera->GetLocalToWorldMatrixXM());
        m_Powerplant.FrustumCulling(frustum);
        m_Cube.FrustumCulling(frustum);
        m_pd3dImmediateContext->RSSetViewports(1, &viewport);

        // 正常绘制
        ID3D11RenderTargetView* pRTVs[1] = { m_pLitBuffer->GetRenderTarget() };
        m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthBuffer->GetDepthStencil());


        m_ForwardEffect.SetCascadeFrustumsEyeSpaceDepths(m_CSManager.GetCascadePartitions());

        // 将NDC空间 [-1, +1]^2 变换到纹理坐标空间 [0, 1]^2
        static XMMATRIX T(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f);
        XMFLOAT4 scales[8]{};
        XMFLOAT4 offsets[8]{};
        for (size_t cascadeIndex = 0; cascadeIndex < m_CSManager.m_CascadeLevels; ++cascadeIndex)
        {
            XMMATRIX ShadowTexture = m_CSManager.GetShadowProjectionXM(cascadeIndex) * T;
            scales[cascadeIndex].x = XMVectorGetX(ShadowTexture.r[0]);
            scales[cascadeIndex].y = XMVectorGetY(ShadowTexture.r[1]);
            scales[cascadeIndex].z = XMVectorGetZ(ShadowTexture.r[2]);
            scales[cascadeIndex].w = 1.0f;
            XMStoreFloat3((XMFLOAT3*)(offsets + cascadeIndex), ShadowTexture.r[3]);
        }
        m_ForwardEffect.SetCascadeOffsets(offsets);
        m_ForwardEffect.SetCascadeScales(scales);
        m_ForwardEffect.SetShadowViewMatrix(m_pLightCamera->GetViewMatrixXM());
        m_ForwardEffect.SetShadowTextureArray(m_CSManager.GetCascadesOutput());
        // 注意：反向Z
        m_ForwardEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), true);
        m_Powerplant.Draw(m_pd3dImmediateContext.Get(), m_ForwardEffect);
        m_Cube.Draw(m_pd3dImmediateContext.Get(), m_ForwardEffect);

        // 清除绑定
        m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
        m_ForwardEffect.SetShadowTextureArray(nullptr);
        m_ForwardEffect.Apply(m_pd3dImmediateContext.Get());
    }
    m_GpuTimer_Lighting.Stop();
}


void GameApp::RenderSkybox()
{
    m_GpuTimer_Skybox.Start();
    {
        D3D11_VIEWPORT skyboxViewport = m_pViewerCamera->GetViewPort();
        skyboxViewport.MinDepth = 1.0f;
        skyboxViewport.MaxDepth = 1.0f;
        m_pd3dImmediateContext->RSSetViewports(1, &skyboxViewport);

        m_SkyboxEffect.SetRenderDefault();
        m_SkyboxEffect.SetLitTexture(m_pLitBuffer->GetShaderResource());
        m_SkyboxEffect.SetDepthTexture(m_pDepthBuffer->GetShaderResource());

        // 由于全屏绘制，不需要用到深度缓冲区，也就不需要清空后备缓冲区了
        ID3D11RenderTargetView* pRTVs[] = { m_EnableFXAA ? m_pTempBuffer->GetRenderTarget() : GetBackBufferRTV()};
        m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
        m_Skybox.Draw(m_pd3dImmediateContext.Get(), m_SkyboxEffect);

        // 清除状态
        m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
        m_SkyboxEffect.SetLitTexture(nullptr);
        m_SkyboxEffect.SetDepthTexture(nullptr);
        m_SkyboxEffect.Apply(m_pd3dImmediateContext.Get());
    }
    m_GpuTimer_Skybox.Stop();
}

void GameApp::PostProcess()
{
    if (m_EnableFXAA)
    {
        m_GpuTimer_PostProcess.Start();
        D3D11_VIEWPORT fullScreenViewport = m_pViewerCamera->GetViewPort();
        fullScreenViewport.MinDepth = 0.0f;
        fullScreenViewport.MaxDepth = 1.0f;
        m_FXAAEffect.RenderFXAA(
            m_pd3dImmediateContext.Get(),
            m_pTempBuffer->GetShaderResource(),
            GetBackBufferRTV(),
            fullScreenViewport);
        m_GpuTimer_PostProcess.Stop();
    }
}

