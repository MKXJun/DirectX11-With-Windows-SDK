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

    m_GpuTimer_PreZ.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    m_GpuTimer_Lighting.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    m_GpuTimer_LightCulling.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    m_GpuTimer_Geometry.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    m_GpuTimer_Skybox.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());

    // 务必先初始化所有渲染状态，以供下面的特效使用
    RenderStates::InitAll(m_pd3dDevice.Get());

    if (!m_ForwardEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_DeferredEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_SkyboxEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{
    D3DApp::OnResize();

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 300.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_ForwardEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM(true));
        m_DeferredEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM(true));
        m_ForwardEffect.SetCameraNearFar(0.5f, 300.0f);
        m_DeferredEffect.SetCameraNearFar(0.5f, 300.0f);
    }

    ResizeBuffers(m_ClientWidth, m_ClientHeight, m_MsaaSamples);
}

void GameApp::UpdateScene(float dt)
{
    // 更新摄像机
    m_FPSCameraController.Update(dt);

#pragma region IMGUI
    bool need_gpu_timer_reset = false;
    if (ImGui::Begin("Tile-Based Deferred Rendering"))
    {
        static const char* msaa_modes[] = {
            "None",
            "2x MSAA",
            "4x MSAA",
            "8x MSAA"
        };
        static int curr_msaa_item = 0;
        if (ImGui::Combo("MSAA", &curr_msaa_item, msaa_modes, ARRAYSIZE(msaa_modes)))
        {
            switch (curr_msaa_item)
            {
            case 0: m_MsaaSamples = 1; break;
            case 1: m_MsaaSamples = 2; break;
            case 2: m_MsaaSamples = 4; break;
            case 3: m_MsaaSamples = 8; break;
            }
            ResizeBuffers(m_ClientWidth, m_ClientHeight, m_MsaaSamples);
            m_ForwardEffect.SetMsaaSamples(m_MsaaSamples);
            m_SkyboxEffect.SetMsaaSamples(m_MsaaSamples);
            m_DeferredEffect.SetMsaaSamples(m_MsaaSamples);
            need_gpu_timer_reset = true;
        }

        static const char* light_culliing_modes[] = {
            "Forward: No Culling",
            "Forward: Pre-Z No Culling",
            "Forward+: Compute Shader Tile",
            "Deferred: No Culling",
            "Deferred: Compute Shader Tile"
        };
        static int curr_light_culliing_item = static_cast<int>(m_LightCullTechnique);
        if (ImGui::Combo("Light Culling", &curr_light_culliing_item, light_culliing_modes, ARRAYSIZE(light_culliing_modes)))
        {
            m_LightCullTechnique = static_cast<LightCullTechnique>(curr_light_culliing_item);
            need_gpu_timer_reset = true;
        }

        if (ImGui::Checkbox("Animate Lights", &m_AnimateLights))
            need_gpu_timer_reset = true;
        if (m_AnimateLights)
        {
            UpdateLights(dt);
        }

        if (ImGui::Checkbox("Lighting Only", &m_LightingOnly))
        {
            m_ForwardEffect.SetLightingOnly(m_LightingOnly);
            m_DeferredEffect.SetLightingOnly(m_LightingOnly);
            need_gpu_timer_reset = true;
        }

        if (ImGui::Checkbox("Face Normals", &m_FaceNormals))
        {
            m_ForwardEffect.SetFaceNormals(m_FaceNormals);
            m_DeferredEffect.SetFaceNormals(m_FaceNormals);
            need_gpu_timer_reset = true;
        }

        if (ImGui::Checkbox("Visualize Light Count", &m_VisualizeLightCount))
        {
            m_ForwardEffect.SetVisualizeLightCount(m_VisualizeLightCount);
            m_DeferredEffect.SetVisualizeLightCount(m_VisualizeLightCount);
            need_gpu_timer_reset = true;
        }

        if (m_LightCullTechnique >= LightCullTechnique::CULL_DEFERRED_NONE)
        {
            ImGui::Checkbox("Clear G-Buffers", &m_ClearGBuffers);
            if (m_MsaaSamples > 1 && ImGui::Checkbox("Visualize Shading Freq", &m_VisualizeShadingFreq))
            {
                m_DeferredEffect.SetVisualizeShadingFreq(m_VisualizeShadingFreq);
                need_gpu_timer_reset = true;
            }
        }
        
        
        ImGui::Text("Light Height Scale");
        ImGui::PushID(0);
        if (ImGui::SliderFloat("", &m_LightHeightScale, 0.25f, 1.0f))
        {
            UpdateLights(0.0f);
            need_gpu_timer_reset = true;
        }
        ImGui::PopID();

        static int light_level = static_cast<int>(log2f(static_cast<float>(m_ActiveLights)));
        ImGui::Text("Lights: %d", m_ActiveLights);
        ImGui::PushID(1);
        if (ImGui::SliderInt("", &light_level, 0, (int)roundf(log2f(MAX_LIGHTS)), ""))
        {
            m_ActiveLights = (1 << light_level);
            ResizeLights(m_ActiveLights);
            UpdateLights(0.0f);
            need_gpu_timer_reset = true;
        }
        ImGui::PopID();
    }
    ImGui::End();

    if (need_gpu_timer_reset)
    {
        m_GpuTimer_PreZ.Reset(m_pd3dImmediateContext.Get());
        m_GpuTimer_Lighting.Reset(m_pd3dImmediateContext.Get());
        m_GpuTimer_LightCulling.Reset(m_pd3dImmediateContext.Get());
        m_GpuTimer_Geometry.Reset(m_pd3dImmediateContext.Get());
        m_GpuTimer_Skybox.Reset(m_pd3dImmediateContext.Get());
    }
#pragma endregion

    m_ForwardEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_DeferredEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    SetupLights(m_pCamera->GetViewMatrixXM());

    //
    // 更新物体与视锥体碰撞
    //
    BoundingFrustum frustum; 
    BoundingFrustum::CreateFromMatrix(frustum, m_pCamera->GetProjMatrixXM());
    frustum.Transform(frustum, m_pCamera->GetLocalToWorldMatrixXM());
    m_Sponza.FrustumCulling(frustum);
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
    if (m_LightCullTechnique == LightCullTechnique::CULL_FORWARD_NONE)
        RenderForward(false);
    else if (m_LightCullTechnique <= LightCullTechnique::CULL_FORWARD_COMPUTE_SHADER_TILE)
        RenderForward(true);
    else {
        RenderGBuffer();

        m_GpuTimer_Lighting.Start();
        if (m_LightCullTechnique == LightCullTechnique::CULL_DEFERRED_NONE)
            m_DeferredEffect.ComputeLightingDefault(m_pd3dImmediateContext.Get(), m_pLitBuffer->GetRenderTarget(),
                m_pDepthBufferReadOnlyDSV.Get(), m_pLightBuffer->GetShaderResource(),
                m_pGBufferSRVs.data(), m_pCamera->GetViewPort());
        else if (m_LightCullTechnique == LightCullTechnique::CULL_DEFERRED_COMPUTE_SHADER_TILE)
            m_DeferredEffect.ComputeTiledLightCulling(m_pd3dImmediateContext.Get(),
                m_pFlatLitBuffer->GetUnorderedAccess(),
                m_pLightBuffer->GetShaderResource(),
                m_pGBufferSRVs.data());
        m_GpuTimer_Lighting.Stop();
    }
    
    RenderSkybox();

    //
    // ImGui部分
    //
    if (ImGui::Begin("Tile-Based Deferred Rendering"))
    {
        ImGui::Separator();
        ImGui::Text("GPU Profile");
        double total_time = 0.0f;
        if (m_LightCullTechnique == LightCullTechnique::CULL_FORWARD_PREZ_NONE || 
            m_LightCullTechnique == LightCullTechnique::CULL_FORWARD_COMPUTE_SHADER_TILE)
        {
            m_GpuTimer_PreZ.TryGetTime(nullptr);
            ImGui::Text("PreZ Pass: %.3f ms", m_GpuTimer_PreZ.AverageTime() * 1000);
            total_time += m_GpuTimer_PreZ.AverageTime();
        }
        if (m_LightCullTechnique == LightCullTechnique::CULL_DEFERRED_NONE || 
            m_LightCullTechnique == LightCullTechnique::CULL_DEFERRED_COMPUTE_SHADER_TILE)
        {
            m_GpuTimer_Geometry.TryGetTime(nullptr);
            ImGui::Text("Geometry Pass: %.3f ms", m_GpuTimer_Geometry.AverageTime() * 1000);
            total_time += m_GpuTimer_Geometry.AverageTime();
        }
        if (m_LightCullTechnique == LightCullTechnique::CULL_FORWARD_COMPUTE_SHADER_TILE)
        {
            m_GpuTimer_LightCulling.TryGetTime(nullptr);
            ImGui::Text("Light Culling Pass: %.3f ms", m_GpuTimer_LightCulling.AverageTime() * 1000);
            total_time += m_GpuTimer_LightCulling.AverageTime();
        }
        if (m_LightCullTechnique < LightCullTechnique::CULL_DEFERRED_COMPUTE_SHADER_TILE)
        {
            m_GpuTimer_Lighting.TryGetTime(nullptr);
            ImGui::Text("Lighting Pass: %.3f ms", m_GpuTimer_Lighting.AverageTime() * 1000);
            total_time += m_GpuTimer_Lighting.AverageTime();
        }
        else
        {
            m_GpuTimer_Lighting.TryGetTime(nullptr);
            ImGui::Text("Culling & Lighting Pass: %.3f ms", m_GpuTimer_Lighting.AverageTime() * 1000);
            total_time += m_GpuTimer_Lighting.AverageTime();
        }

        m_GpuTimer_Skybox.TryGetTime(nullptr);
        ImGui::Text("Skybox Pass: %.3f ms", m_GpuTimer_Skybox.AverageTime() * 1000);
        total_time += m_GpuTimer_Skybox.AverageTime();

        ImGui::Text("Total: %.3f ms", total_time * 1000);
    }
    ImGui::End();

    if (m_LightCullTechnique >= LightCullTechnique::CULL_DEFERRED_NONE)
    {
        if (ImGui::Begin("Normal"))
        {
            m_DeferredEffect.DebugNormalGBuffer(m_pd3dImmediateContext.Get(), m_pDebugNormalGBuffer->GetRenderTarget(),
                m_pGBufferSRVs[0], m_pCamera->GetViewPort());
            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
            ImGui::Image(m_pDebugNormalGBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
        }
        ImGui::End();

        if (ImGui::Begin("Albedo"))
        {
            m_pd3dImmediateContext->ResolveSubresource(m_pDebugAlbedoGBuffer->GetTexture(), 0, m_pGBuffers[1]->GetTexture(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
            ImGui::Image(m_pDebugAlbedoGBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
        }
        ImGui::End();

        if (ImGui::Begin("PosZGrad"))
        {

            m_DeferredEffect.DebugPosZGradGBuffer(m_pd3dImmediateContext.Get(), m_pDebugPosZGradGBuffer->GetRenderTarget(),
                m_pGBufferSRVs[2], m_pCamera->GetViewPort());
            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
            ImGui::Image(m_pDebugPosZGradGBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
        }
        ImGui::End();
    }
    
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

    auto camera = std::make_shared<FirstPersonCamera>();
    m_pCamera = camera;

    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 300.0f);
    camera->LookAt(XMFLOAT3(-60.0f, 10.0f, 2.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    m_FPSCameraController.InitCamera(camera.get());
    m_FPSCameraController.SetMoveSpeed(10.0f);
    // ******************
    // 初始化特效
    //

    m_ForwardEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_DeferredEffect.SetViewMatrix(camera->GetViewMatrixXM());
    // 注意：反转Z
    m_ForwardEffect.SetProjMatrix(camera->GetProjMatrixXM(true));
    m_DeferredEffect.SetProjMatrix(camera->GetProjMatrixXM(true));
    m_ForwardEffect.SetCameraNearFar(0.5f, 300.0f);
    m_DeferredEffect.SetCameraNearFar(0.5f, 300.0f);
    m_ForwardEffect.SetMsaaSamples(1);
    m_DeferredEffect.SetMsaaSamples(1);
    m_SkyboxEffect.SetMsaaSamples(1);

    // ******************
    // 初始化天空盒纹理
    //
    m_TextureManager.CreateFromFile("..\\Texture\\Clouds.dds");

    // ******************
    // 初始化对象
    //
    m_Sponza.SetModel(m_ModelManager.CreateFromFile("..\\Model\\Sponza\\sponza.gltf"));
    m_Sponza.GetTransform().SetScale(0.05f, 0.05f, 0.05f);
    m_ModelManager.CreateFromGeometry("skyboxCube", Geometry::CreateBox());
    Model* pModel = m_ModelManager.GetModel("skyboxCube"); 
    pModel->materials[0].Set<std::string>("$Skybox", "..\\Texture\\Clouds.dds");
    m_Skybox.SetModel(pModel);

    // ******************
    // 初始化光照
    //

    InitLightParams();
    ResizeLights(m_ActiveLights);
    UpdateLights(0.0f);

    return true;
}

void GameApp::InitLightParams()
{
    m_PointLightParams.resize(MAX_LIGHTS);
    m_PointLightInitDatas.resize(MAX_LIGHTS);
    m_PointLightPosWorlds.resize(MAX_LIGHTS);

    // 使用固定的随机数种子保持一致性
    std::mt19937 rng(1337);
    constexpr float maxRadius = 100.0f;
    constexpr float attenuationStartFactor = 0.8f;
    std::uniform_real<float> radiusNormDist(0.0f, 1.0f);
    std::uniform_real<float> angleDist(0.0f, 2.0f * XM_PI);
    std::uniform_real<float> heightDist(0.0f, 75.0f);
    std::uniform_real<float> animationSpeedDist(2.0f, 20.0f);
    std::uniform_int<int> animationDirection(0, 1);
    std::uniform_real<float> hueDist(0.0f, 1.0f);
    std::uniform_real<float> intensityDist(0.2f, 0.8f);
    std::uniform_real<float> attenuationDist(2.0f, 20.0f);

    for (unsigned int i = 0; i < MAX_LIGHTS; ++i) {
        PointLight& params = m_PointLightParams[i];
        PointLightInitData& data = m_PointLightInitDatas[i];

        data.radius = std::sqrt(radiusNormDist(rng)) * maxRadius;
        data.angle = angleDist(rng);
        data.height = heightDist(rng);
        // 归一化速度
        data.animationSpeed = (animationDirection(rng) * 2 - 1) * animationSpeedDist(rng) / data.radius;

        // HSL->RGB
        params.color = HueToRGB(hueDist(rng));
        XMStoreFloat3(&params.color, XMLoadFloat3(&params.color) * intensityDist(rng));
        params.attenuationEnd = attenuationDist(rng);
        params.attenuationBegin = attenuationStartFactor * params.attenuationEnd;
    }
}

XMFLOAT3 GameApp::HueToRGB(float hue)
{
    float intPart;
    float fracPart = std::modf(hue * 6.0f, &intPart);
    int region = static_cast<int>(intPart);

    switch (region)
    {
    case 0: return XMFLOAT3(1.0f, fracPart, 0.0f);
    case 1: return XMFLOAT3(1.0f - fracPart, 1.0f, 0.0f);
    case 2: return XMFLOAT3(0.0f, 1.0f, fracPart);
    case 3: return XMFLOAT3(0.0f, 1.0f - fracPart, 1.0f);
    case 4: return XMFLOAT3(fracPart, 0.0f, 1.0f);
    case 5: return XMFLOAT3(1.0f, 0.0f, 1.0f - fracPart);
    }
    return DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void GameApp::ResizeLights(UINT activeLights)
{
    m_ActiveLights = activeLights;
    m_pLightBuffer = std::make_unique<StructuredBuffer<PointLight>>(m_pd3dDevice.Get(), activeLights, D3D11_BIND_SHADER_RESOURCE, false, true);

    // ******************
    // 设置调试对象名
    //
    m_pLightBuffer->SetDebugObjectName("LightBuffer");
}

void GameApp::UpdateLights(float dt)
{
    static float totalTime = 0.0f;
    totalTime += dt;
    for (UINT i = 0; i < m_ActiveLights; ++i)
    {
        const auto& data = m_PointLightInitDatas[i];
        float angle = data.angle + totalTime * data.animationSpeed;
        m_PointLightPosWorlds[i] = XMFLOAT3(
            data.radius * std::cos(angle),
            data.height * m_LightHeightScale,
            data.radius * std::sin(angle)
        );
    }
}

void XM_CALLCONV GameApp::SetupLights(DirectX::XMMATRIX viewMatrix)
{
    XMVector3TransformCoordStream(&m_PointLightParams[0].posV, sizeof(PointLight),
        &m_PointLightPosWorlds[0], sizeof(XMFLOAT3), m_ActiveLights, viewMatrix);

    PointLight* pData = m_pLightBuffer->MapDiscard(m_pd3dImmediateContext.Get());
    memcpy_s(pData, sizeof(PointLight) * m_ActiveLights,
        m_PointLightParams.data(), sizeof(PointLight) * m_ActiveLights);
    m_pLightBuffer->Unmap(m_pd3dImmediateContext.Get());
}

void GameApp::ResizeBuffers(UINT width, UINT height, UINT msaaSamples)
{
    // ******************
    // 初始化前向渲染所需资源
    //
    UINT tileWidth = (width + COMPUTE_SHADER_TILE_GROUP_DIM - 1) / COMPUTE_SHADER_TILE_GROUP_DIM;
    UINT tileHeight = (height + COMPUTE_SHADER_TILE_GROUP_DIM - 1) / COMPUTE_SHADER_TILE_GROUP_DIM;
    m_pTileBuffer = std::make_unique<StructuredBuffer<TileInfo>>(m_pd3dDevice.Get(), 
        width * height, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);

    // ******************
    // 初始化延迟渲染所需资源
    //
    DXGI_SAMPLE_DESC sampleDesc;
    sampleDesc.Count = msaaSamples;
    sampleDesc.Quality = 0;
    m_pLitBuffer = std::make_unique<Texture2DMS>(m_pd3dDevice.Get(), width, height,
        DXGI_FORMAT_R16G16B16A16_FLOAT, sampleDesc,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

    m_pFlatLitBuffer = std::make_unique<StructuredBuffer<DirectX::XMUINT2>>(
        m_pd3dDevice.Get(), width * height * msaaSamples,
        D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
    
    m_pDepthBuffer = std::make_unique<Depth2DMS>(m_pd3dDevice.Get(), width, height, sampleDesc,
        // 使用MSAA则需要提供模板
        m_MsaaSamples > 1 ? DepthStencilBitsFlag::Depth_32Bits_Stencil_8Bits_Unused_24Bits : DepthStencilBitsFlag::Depth_32Bits,
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);

    // 创建只读深度/模板视图
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        m_pDepthBuffer->GetDepthStencil()->GetDesc(&desc);
        desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
        m_pd3dDevice->CreateDepthStencilView(m_pDepthBuffer->GetTexture(), &desc, m_pDepthBufferReadOnlyDSV.ReleaseAndGetAddressOf());
    }

    // G-Buffer
    // MRT要求所有的G-Buffer使用相同的MSAA采样等级
    m_pGBuffers.clear();
    // normal_specular
    // normal_specular
    m_pGBuffers.push_back(std::make_unique<Texture2DMS>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, sampleDesc,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE));
    // albedo
    m_pGBuffers.push_back(std::make_unique<Texture2DMS>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, sampleDesc,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE));
    // posZgrad
    m_pGBuffers.push_back(std::make_unique<Texture2DMS>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R16G16_FLOAT, sampleDesc,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE));

    // 设置GBuffer资源列表
    m_pGBufferRTVs.resize(m_pGBuffers.size(), 0);
    m_pGBufferSRVs.resize(4, 0);
    for (std::size_t i = 0; i < m_pGBuffers.size(); ++i) {
        m_pGBufferRTVs[i] = m_pGBuffers[i]->GetRenderTarget();
        m_pGBufferSRVs[i] = m_pGBuffers[i]->GetShaderResource();
    }
    // 深度缓冲区作为最后的SRV用于读取
    m_pGBufferSRVs.back() = m_pDepthBuffer->GetShaderResource();

    // 调试用缓冲区
    m_pDebugNormalGBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDebugAlbedoGBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDebugPosZGradGBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R16G16_FLOAT);
    // ******************
    // 设置调试对象名
    //

#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(m_pDepthBufferReadOnlyDSV.Get(), "DepthBufferReadOnlyDSV");
    m_pDepthBuffer->SetDebugObjectName("DepthBuffer");
    m_pLitBuffer->SetDebugObjectName("LitBuffer");
    m_pFlatLitBuffer->SetDebugObjectName("FlatLitBuffer");
    m_pGBuffers[0]->SetDebugObjectName("GBuffer_Normal_Specular");
    m_pGBuffers[1]->SetDebugObjectName("GBuffer_Albedo");
    m_pGBuffers[2]->SetDebugObjectName("GBuffer_PosZgrad");
#endif

}

void GameApp::RenderForward(bool doPreZ)
{
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pd3dImmediateContext->ClearRenderTargetView(m_pLitBuffer->GetRenderTarget(), black);
    // 注意：反向Z的缓冲区，远平面为0
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthBuffer->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

    D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    // PreZ Pass
    if (doPreZ)
    {
        m_GpuTimer_PreZ.Start();
        m_pd3dImmediateContext->OMSetRenderTargets(0, 0, m_pDepthBuffer->GetDepthStencil());
        m_ForwardEffect.SetRenderPreZPass();
        m_Sponza.Draw(m_pd3dImmediateContext.Get(), m_ForwardEffect);
        m_pd3dImmediateContext->OMSetRenderTargets(0, 0, nullptr);
        m_GpuTimer_PreZ.Stop();
    }

    // 光源裁剪阶段
    if (m_LightCullTechnique == LightCullTechnique::CULL_FORWARD_COMPUTE_SHADER_TILE)
    {
        m_GpuTimer_LightCulling.Start();
        m_ForwardEffect.ComputeTiledLightCulling(m_pd3dImmediateContext.Get(),
            m_pTileBuffer->GetUnorderedAccess(),
            m_pLightBuffer->GetShaderResource(),
            m_pGBufferSRVs[3]);
        m_GpuTimer_LightCulling.Stop();
    }

    // 正常绘制
    m_GpuTimer_Lighting.Start();
    {
        ID3D11RenderTargetView* pRTVs[1] = { m_pLitBuffer->GetRenderTarget() };
        m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthBuffer->GetDepthStencil());

        if (m_LightCullTechnique == LightCullTechnique::CULL_FORWARD_COMPUTE_SHADER_TILE)
        {
            m_ForwardEffect.SetTileBuffer(m_pTileBuffer->GetShaderResource());
            m_ForwardEffect.SetRenderWithTiledLightCulling();
        }
        else
            m_ForwardEffect.SetRenderDefault();

        m_ForwardEffect.SetLightBuffer(m_pLightBuffer->GetShaderResource());
        m_Sponza.Draw(m_pd3dImmediateContext.Get(), m_ForwardEffect);

        // 清除绑定
        m_ForwardEffect.SetTileBuffer(nullptr);
        m_ForwardEffect.Apply(m_pd3dImmediateContext.Get());
        m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
    }
    m_GpuTimer_Lighting.Stop();
}

void GameApp::RenderGBuffer()
{
    // 注意：我们实际上只需要清空深度缓冲区，因为我们用天空盒的采样来替代没有写入的像素(例如：远平面)
    // 	    这里我们使用深度缓冲区来重建位置且只有在视锥体内的位置会被着色
    // 注意：反转Z的缓冲区，远平面为0
    if (m_ClearGBuffers)
    {
        float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        for (auto rtv : m_pGBufferRTVs)
            m_pd3dImmediateContext->ClearRenderTargetView(rtv, black);
    }
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthBuffer->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);
    
    D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    m_GpuTimer_Geometry.Start();
    {
        m_DeferredEffect.SetRenderGBuffer();
        m_pd3dImmediateContext->OMSetRenderTargets(static_cast<UINT>(m_pGBuffers.size()), m_pGBufferRTVs.data(), m_pDepthBuffer->GetDepthStencil());
        m_Sponza.Draw(m_pd3dImmediateContext.Get(), m_DeferredEffect);

        m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
    }
    m_GpuTimer_Geometry.Stop();
}

void GameApp::RenderSkybox()
{
    m_GpuTimer_Skybox.Start();
    {
        D3D11_VIEWPORT skyboxViewport = m_pCamera->GetViewPort();
        skyboxViewport.MinDepth = 1.0f;
        skyboxViewport.MaxDepth = 1.0f;
        m_pd3dImmediateContext->RSSetViewports(1, &skyboxViewport);

        m_SkyboxEffect.SetRenderDefault();

        m_SkyboxEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
        // 注意：反转Z
        m_SkyboxEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM(true));

        // 根据所用技术绑定合适的缓冲区
        if (m_LightCullTechnique == LightCullTechnique::CULL_DEFERRED_COMPUTE_SHADER_TILE)
            m_SkyboxEffect.SetFlatLitTexture(m_pFlatLitBuffer->GetShaderResource(), m_ClientWidth, m_ClientHeight);
        else
            m_SkyboxEffect.SetLitTexture(m_pLitBuffer->GetShaderResource());
        m_SkyboxEffect.SetDepthTexture(m_pDepthBuffer->GetShaderResource());
        m_SkyboxEffect.Apply(m_pd3dImmediateContext.Get());

        // 由于全屏绘制，不需要用到深度缓冲区，也就不需要清空后备缓冲区了
        ID3D11RenderTargetView* pRTVs[] = { GetBackBufferRTV() };
        m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
        m_Skybox.Draw(m_pd3dImmediateContext.Get(), m_SkyboxEffect);

        // 清除状态
        m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
        m_SkyboxEffect.SetLitTexture(nullptr);
        m_SkyboxEffect.SetFlatLitTexture(nullptr, 0, 0);
        m_SkyboxEffect.SetDepthTexture(nullptr);
        m_SkyboxEffect.Apply(m_pd3dImmediateContext.Get());
    }
    m_GpuTimer_Skybox.Stop();
}

