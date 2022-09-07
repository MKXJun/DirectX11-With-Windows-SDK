//#define GRAPHICS_DEBUG

#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <DirectXPackedVector.h>
using namespace DirectX;
namespace fs = std::filesystem;

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

    m_GpuTimer_Shadow.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(), 2000);
    m_GpuTimer_Lighting.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(), 2000);
    m_GpuTimer_Skybox.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(), 2000);
    m_GpuTimer_PostProcess.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(), 2000);
    m_GpuTimer_Geometry.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(), 2000);

    // 务必先初始化所有渲染状态，以供下面的特效使用
    RenderStates::InitAll(m_pd3dDevice.Get());


    if (!m_DeferredEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_ShadowEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_SkyboxToneMapEffect.InitAll(m_pd3dDevice.Get()))
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

    m_pLitBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS);
    m_pDepthBuffer = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DepthStencilBitsFlag::Depth_32Bits);
    m_pTempBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDebugNormalBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDebugAlbedoBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDebugMetallicBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDebugRoughnessBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

    // 创建只读深度/模板视图
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        m_pDepthBuffer->GetDepthStencil()->GetDesc(&desc);
        desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
        m_pd3dDevice->CreateDepthStencilView(m_pDepthBuffer->GetTexture(), &desc, m_pDepthBufferReadOnlyDSV.ReleaseAndGetAddressOf());
    }

    // G-Buffer
    m_pGBuffers.clear();
    // normal_roughness
    m_pGBuffers.push_back(std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT));
    // albedo_metallic
    m_pGBuffers.push_back(std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM));
    
    // 设置GBuffer资源列表
    m_pGBufferRTVs.resize(m_pGBuffers.size(), 0);
    m_pGBufferSRVs.resize(m_pGBuffers.size() + 1, 0);
    for (std::size_t i = 0; i < m_pGBuffers.size(); ++i) {
        m_pGBufferRTVs[i] = m_pGBuffers[i]->GetRenderTarget();
        m_pGBufferSRVs[i] = m_pGBuffers[i]->GetShaderResource();
    }
    // 深度缓冲区作为最后的SRV用于读取
    m_pGBufferSRVs.back() = m_pDepthBuffer->GetShaderResource();

    // ******************
    // 设置调试对象名
    //
    
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(m_pDepthBufferReadOnlyDSV.Get(), "DepthBufferReadOnlyDSV");
    m_pDepthBuffer->SetDebugObjectName("DepthBuffer");
    m_pLitBuffer->SetDebugObjectName("LitBuffer");
    m_pTempBuffer->SetDebugObjectName("TempBuffer");
    m_pGBuffers[0]->SetDebugObjectName("GBuffer_Normal_Roughness");
    m_pGBuffers[1]->SetDebugObjectName("GBuffer_Albedo_Metallic");
#endif

    // 摄像机变更显示
    if (m_pViewerCamera != nullptr)
    {
        m_pViewerCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 300.0f);
        m_pViewerCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        // 注意：反向Z
        m_DeferredEffect.SetProjMatrix(m_pViewerCamera->GetProjMatrixXM(true));
    }
}

void GameApp::UpdateScene(float dt)
{
    // 更新摄像机
    m_FPSCameraController.Update(dt);

    UpdateImGui(dt);

    // 注意：反向Z
    m_DeferredEffect.SetViewMatrix(m_pViewerCamera->GetViewMatrixXM());
    m_DeferredEffect.SetProjMatrix(m_pViewerCamera->GetProjMatrixXM(true));
    m_SkyboxToneMapEffect.SetViewMatrix(m_pViewerCamera->GetViewMatrixXM());
    m_SkyboxToneMapEffect.SetProjMatrix(m_pViewerCamera->GetProjMatrixXM(true));


    // 更新方向光
    XMFLOAT3 dirLightDir = m_pLightCamera->GetLookAxis();
    XMStoreFloat3(&dirLightDir, XMVector3TransformNormal(XMLoadFloat3(&dirLightDir), m_pViewerCamera->GetViewMatrixXM()));
    m_DeferredEffect.SetDirectionalLight(dirLightDir, m_DirLightColor, m_DirLightIntensity);

    // 更新点光源
    memcpy_s(m_UploadPointLights.data(), m_NumPointLights * sizeof(PointLight),
        m_PointLights.data(), m_NumPointLights * sizeof(PointLight));
    XMVector3TransformCoordStream(&m_UploadPointLights[0].pos, sizeof(PointLight), 
        &m_PointLights[0].pos, sizeof(PointLight), m_NumPointLights, m_pViewerCamera->GetViewMatrixXM());
    memset(m_UploadPointLights.data() + m_NumPointLights, 0, (m_UploadPointLights.size() - (size_t)m_NumPointLights) * sizeof(PointLight));
    memcpy_s(m_pPointLightBuffer->MapDiscard(m_pd3dImmediateContext.Get()), m_pPointLightBuffer->GetByteWidth(),
        m_UploadPointLights.data(), m_pPointLightBuffer->GetByteWidth());
    m_pPointLightBuffer->Unmap(m_pd3dImmediateContext.Get());
  

    // 更新阴影
    m_ShadowEffect.SetViewMatrix(m_pLightCamera->GetViewMatrixXM());
    m_CSManager.UpdateFrame(*m_pViewerCamera, *m_pLightCamera, m_Sponza.GetModel()->boundingbox);
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
    RenderGBuffer();

    // 光源剔除与绘制
    m_GpuTimer_Lighting.Start();
    {

        m_DeferredEffect.SetCascadeFrustumsEyeSpaceDepths(m_CSManager.GetCascadePartitions());

        // 将NDC空间 [-1, +1]^2 变换到纹理坐标空间 [0, 1]^2
        static XMMATRIX T(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f);
        XMFLOAT4 scales[4]{};
        XMFLOAT4 offsets[4]{};
        for (size_t cascadeIndex = 0; cascadeIndex < m_CSManager.m_CascadeLevels; ++cascadeIndex)
        {
            XMMATRIX ShadowTexture = m_CSManager.GetShadowProjectionXM(cascadeIndex) * T;
            scales[cascadeIndex].x = XMVectorGetX(ShadowTexture.r[0]);
            scales[cascadeIndex].y = XMVectorGetY(ShadowTexture.r[1]);
            scales[cascadeIndex].z = XMVectorGetZ(ShadowTexture.r[2]);
            scales[cascadeIndex].w = 1.0f;
            XMStoreFloat3((XMFLOAT3*)(offsets + cascadeIndex), ShadowTexture.r[3]);
        }
        m_DeferredEffect.SetCascadeOffsets(offsets);
        m_DeferredEffect.SetCascadeScales(scales);
        m_DeferredEffect.SetShadowViewMatrix(m_pLightCamera->GetViewMatrixXM());

        m_DeferredEffect.ComputeTiledLightCulling(m_pd3dImmediateContext.Get(),
            m_pLitBuffer->GetUnorderedAccess(),
            m_pPointLightBuffer->GetShaderResource(),
            m_CSManager.GetCascadesOutput(),
            m_pGBufferSRVs.data());
    }
    m_GpuTimer_Lighting.Stop();

    RenderSkybox();
    PostProcess();

    DrawImGui();

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}

bool GameApp::InitResource()
{
    // ******************
    // 初始化摄像机和控制器
    //

    m_pViewerCamera = std::make_shared<FirstPersonCamera>();

    m_pViewerCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    m_pViewerCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 300.0f);
    m_pViewerCamera->LookAt(XMFLOAT3(-6.0f, 4.0f, 4.0f), XMFLOAT3(4.0f, 4.0f, 4.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    m_FPSCameraController.InitCamera(m_pViewerCamera.get());
    m_FPSCameraController.SetMoveSpeed(10.0f);

    m_FPSCameraController.InitCamera(m_pViewerCamera.get());
    m_FPSCameraController.SetMoveSpeed(10.0f);
    
    // ******************
    // 初始化对象
    //

    m_TextureManager.CreateFromFile("..\\Texture\\rustediron2_basecolor.png", 
        TextureManager::LoadConfig_ForceSRGB | TextureManager::LoadConfig_EnableMips);
    m_TextureManager.CreateFromFile("..\\Texture\\rustediron2_metallic.png");
    m_TextureManager.CreateFromFile("..\\Texture\\rustediron2_normal.png");
    m_TextureManager.CreateFromFile("..\\Texture\\rustediron2_roughness.png");

    Model* pModel = m_ModelManager.CreateFromFile("..\\Model\\SponzaPBR\\sponza.gltf");
    // 去掉有问题的子模型
    pModel->meshdatas.erase(std::remove_if(pModel->meshdatas.begin(), pModel->meshdatas.end(), [](const MeshData& meshData) {
        return meshData.m_IndexCount == 54;
        }));
    m_Sponza.SetModel(pModel);
    m_Sponza.GetTransform().SetScale(0.05f, 0.05f, 0.05f);


    pModel = m_ModelManager.CreateFromGeometry("Sphere", Geometry::CreateSphere());
    pModel->materials[0].Set<std::string>("$Albedo", "..\\Texture\\rustediron2_basecolor.png");
    pModel->materials[0].Set<std::string>("$Metallic", "..\\Texture\\rustediron2_metallic.png");
    pModel->materials[0].Set<std::string>("$Normal", "..\\Texture\\rustediron2_normal.png");
    pModel->materials[0].Set<std::string>("$Roughness", "..\\Texture\\rustediron2_roughness.png");
    m_Sphere.SetModel(pModel);
    m_Sphere.GetTransform().SetPosition(4.0f, 4.0f, 4.0f);
    m_Sphere.GetTransform().SetScale(2.0f, 2.0f, 2.0f);
    
    // ******************
    // 初始化天空盒相关

    ComPtr<ID3D11Texture2D> pTex;
    D3D11_TEXTURE2D_DESC texDesc;
    std::string filenameStr;
    std::vector<ID3D11ShaderResourceView*> pCubeTextures;
    std::unique_ptr<TextureCube> pTexCube;
    // Daylight
    {
        filenameStr = "..\\Texture\\daylight0.png";
        for (size_t i = 0; i < 6; ++i)
        {
            filenameStr[19] = '0' + (char)i;
            pCubeTextures.push_back(m_TextureManager.CreateFromFile(filenameStr));
        }

        pCubeTextures[0]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.ReleaseAndGetAddressOf()));
        pTex->GetDesc(&texDesc);
        pTexCube = std::make_unique<TextureCube>(m_pd3dDevice.Get(), texDesc.Width, texDesc.Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        pTexCube->SetDebugObjectName("Daylight");
        for (uint32_t i = 0; i < 6; ++i)
        {
            pCubeTextures[i]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.ReleaseAndGetAddressOf()));
            m_pd3dImmediateContext->CopySubresourceRegion(pTexCube->GetTexture(), D3D11CalcSubresource(0, i, 1), 0, 0, 0, pTex.Get(), 0, nullptr);
        }
        m_TextureManager.AddTexture("Daylight", pTexCube->GetShaderResource());
    }
    
    // ******************
    // 初始化阴影
    //
    m_CSManager.InitResource(m_pd3dDevice.Get());

    // ******************
    // 初始化灯光
    //

    // 方向光
    m_pLightCamera = std::make_shared<FirstPersonCamera>();

    m_pLightCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    m_pLightCamera->SetRotation(XM_PI / 9 * 4, -XM_PI / 9 * 2, 0.0f);
    m_pLightCamera->SetFrustum(XM_PI / 3, 1.0f, 0.1f, 1000.0f);

    m_DirLightColor = XMFLOAT3(1.0f, 0.956862f, 0.839215f);
    m_DirLightIntensity = XM_PI;

    // 点光源
    m_NumPointLights = 5;
    m_pPointLightBuffer = std::make_unique<StructuredBuffer<PointLight>>(m_pd3dDevice.Get(), 256, D3D11_BIND_SHADER_RESOURCE, false, true);
    m_UploadPointLights.resize(256);
    m_PointLights.resize(256);
    m_PointLights[0] = PointLight{ XMFLOAT3(-56.77f, 36.15f, -19.86f), 120.0f, XMFLOAT3(0.8867924f, 0.3137237f, 0.3137237f), 150.0f };
    m_PointLights[1] = PointLight{ XMFLOAT3(50.88f, 36.15f, -19.66f), 120.0f, XMFLOAT3(0.3254902f, 0.5411765f, 0.8784314f), 150.0f };
    m_PointLights[2] = PointLight{ XMFLOAT3(-1.401f, 6.074f, -19.93f), 120.0f, XMFLOAT3(1.0f, 1.0f, 1.0f), 100.0f };
    m_PointLights[3] = PointLight{ XMFLOAT3(-60.5f, 11.22f, 1.73f), 60.0f, XMFLOAT3(0.2525692f, 0.8679245f, 0.233357f), 150.0f };
    m_PointLights[4] = PointLight{ XMFLOAT3(54.42f, 11.22f, 1.73f), 60.0f, XMFLOAT3(0.8301887f, 0.48792f, 0.1135636f), 150.0f };

    // ******************
    // 初始化特效
    //

    m_DeferredEffect.SetViewMatrix(m_pViewerCamera->GetViewMatrixXM());
    m_DeferredEffect.SetProjMatrix(m_pViewerCamera->GetProjMatrixXM(true));
    m_DeferredEffect.SetCameraNearFar(0.5f, 300.0f);

    m_DeferredEffect.SetShadowSize(m_CSManager.m_ShadowSize);
    m_DeferredEffect.SetCascadeBlendArea(m_CSManager.m_BlendBetweenCascadesRange);
    m_DeferredEffect.SetPCFKernelSize(m_CSManager.m_BlurKernelSize);
    m_DeferredEffect.SetPCFDepthBias(m_CSManager.m_PCFDepthBias);

    m_FXAAEffect.SetQualitySubPix(0.75f);
    m_FXAAEffect.SetQualityEdgeThreshold(0.166f);
    m_FXAAEffect.SetQualityEdgeThresholdMin(0.0833f);
    m_FXAAEffect.SetQuality(3, 9);

    m_ShadowEffect.SetViewMatrix(m_pLightCamera->GetViewMatrixXM());

    return true;
}


void GameApp::UpdateImGui(float dt)
{
    if (ImGui::Begin("PBR"))
    { 
        if (ImGui::CollapsingHeader("Settings"))
        {
            ImGui::Checkbox("Enable Scene Normalmap", &m_EnableNormalMap);
            if (m_EnableNormalMap)
            {
                ImGui::Checkbox("Use Tangent", &m_UseTangent);
            }

            // ToneMapping
            static const char* strs[] = {
                "Standard",
                "Reinhard",
                "ACES Coarse",
                "ACES"
            };
            ImGui::Combo("Tone Mapping", (int*)&m_ToneMapping, strs, ARRAYSIZE(strs));

            ImGui::Checkbox("Debug Normal", &m_DebugNormal);
            ImGui::Checkbox("Debug Albedo", &m_DebugAlbedo);
            ImGui::Checkbox("Debug Roughness", &m_DebugRoughness);
            ImGui::Checkbox("Debug Metallic", &m_DebugMetallic);
        }

        if (ImGui::CollapsingHeader("Directional Light"))
        {
            static XMFLOAT3 rotationAngle = XMFLOAT3(80.0f, -40.0f, 0.0f);
            if (ImGui::InputFloat3("Euler Angles", reinterpret_cast<float*>(&rotationAngle)))
                m_pLightCamera->SetRotation(rotationAngle.x / 180 * XM_PI, rotationAngle.y / 180 * XM_PI, rotationAngle.z / 180 * XM_PI);
            ImGui::ColorEdit3("Color##dirLight", reinterpret_cast<float*>(&m_DirLightColor));
            ImGui::InputFloat("Intensity##dirLight", &m_DirLightIntensity);
            if (m_DirLightIntensity < 0.0f)
                m_DirLightIntensity = 0.0f;
        }

        if (ImGui::CollapsingHeader("Point Light"))
        {
            ImGui::InputInt("Num Lights", &m_NumPointLights);
            m_NumPointLights = std::clamp(m_NumPointLights, 0, 256);

            std::string idxStr;
            for (int i = 0; i < m_NumPointLights; ++i)
            {
                idxStr = "Light #" + std::to_string(i);
                ImGui::PushID(i);
                ImGui::Text(idxStr.c_str());
                ImGui::InputFloat3("Position", reinterpret_cast<float*>(&m_PointLights[i].pos));
                ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&m_PointLights[i].color));
                ImGui::InputFloat("Range", &m_PointLights[i].range);
                ImGui::InputFloat("Intensity", &m_PointLights[i].intensity);
                if (m_PointLights[i].intensity < 0.0f)
                    m_PointLights[i].intensity = 0.0f;
                ImGui::PopID();
                ImGui::Separator();
            }
        }

        if (ImGui::CollapsingHeader("Sphere"))
        {
            static XMFLOAT3 spherePos = m_Sphere.GetTransform().GetPosition();
            if (ImGui::InputFloat3("Position", (float*)&spherePos))
                m_Sphere.GetTransform().SetPosition(spherePos);
            if (ImGui::Checkbox("Use Texture", &m_UseTexture))
            {
                Model* pModel = m_ModelManager.GetModel("Sphere");
                if (m_UseTexture)
                {
                    pModel->materials[0].Set<std::string>("$Albedo", "..\\Texture\\rustediron2_basecolor.png");
                    pModel->materials[0].Set<std::string>("$Metallic", "..\\Texture\\rustediron2_metallic.png");
                    pModel->materials[0].Set<std::string>("$Normal", "..\\Texture\\rustediron2_normal.png");
                    pModel->materials[0].Set<std::string>("$Roughness", "..\\Texture\\rustediron2_roughness.png");
                }
                else
                {
                    pModel->materials[0].Set<DirectX::XMFLOAT4>("$Albedo", m_SphereAlbedo);
                    pModel->materials[0].Set<float>("$Metallic", m_SphereMetallic);
                    pModel->materials[0].Set<std::string>("$Normal", "$Normal");
                    pModel->materials[0].Set<float>("$Roughness", m_SphereRoughness);
                }
            }
            if (!m_UseTexture)
            {
                ImGui::ColorEdit4("Color##Sphere", (float*)&m_SphereAlbedo);
                ImGui::SliderFloat("Metallic", &m_SphereMetallic, 0.0f, 1.0f);
                ImGui::SliderFloat("Roughness", &m_SphereRoughness, 0.0f, 1.0f);

                Model* pModel = m_ModelManager.GetModel("Sphere");
                pModel->materials[0].Set<DirectX::XMFLOAT4>("$Albedo", m_SphereAlbedo);
                pModel->materials[0].Set<float>("$Metallic", m_SphereMetallic);
                pModel->materials[0].Set<float>("$Roughness", m_SphereRoughness);
            }
        }

        ImGui::Separator();
        ImGui::Text("GPU Profile");
        double total_time = 0.0f;

        m_GpuTimer_Geometry.TryGetTime(nullptr);
        ImGui::Text("Geometry Pass: %.3f ms", m_GpuTimer_Geometry.AverageTime() * 1000);
        total_time += m_GpuTimer_Geometry.AverageTime();

        m_GpuTimer_Lighting.TryGetTime(nullptr);
        ImGui::Text("Lighting Pass: %.3f ms", m_GpuTimer_Lighting.AverageTime() * 1000);
        total_time += m_GpuTimer_Lighting.AverageTime();

        m_GpuTimer_Skybox.TryGetTime(nullptr);
        ImGui::Text("Skybox Pass: %.3f ms", m_GpuTimer_Skybox.AverageTime() * 1000);
        total_time += m_GpuTimer_Skybox.AverageTime();

        m_GpuTimer_PostProcess.TryGetTime(nullptr);
        ImGui::Text("PostProcess Pass: %.3f ms", m_GpuTimer_PostProcess.AverageTime() * 1000);
        total_time += m_GpuTimer_PostProcess.AverageTime();

        ImGui::Text("Total: %.3f ms", total_time * 1000);
    }
    ImGui::End();
}

void GameApp::DrawImGui()
{
    if (m_DebugNormal)
    {
        if (ImGui::Begin("Normal", &m_DebugNormal))
        {
            CD3D11_VIEWPORT vp(0.0f, 0.0f, (float)m_pDebugNormalBuffer->GetWidth(), (float)m_pDebugNormalBuffer->GetHeight());
            m_DeferredEffect.DebugNormalGBuffer(m_pd3dImmediateContext.Get(), m_pDebugNormalBuffer->GetRenderTarget(), m_pGBufferSRVs[0], vp);
            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
            ImGui::Image(m_pDebugNormalBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
        }
        ImGui::End();
    }

    if (m_DebugAlbedo)
    {
        if (ImGui::Begin("Albedo", &m_DebugAlbedo))
        {
            CD3D11_VIEWPORT vp(0.0f, 0.0f, (float)m_pDebugAlbedoBuffer->GetWidth(), (float)m_pDebugAlbedoBuffer->GetHeight());
            m_DeferredEffect.DebugAlbedoGBuffer(m_pd3dImmediateContext.Get(), m_pDebugAlbedoBuffer->GetRenderTarget(), m_pGBufferSRVs[1], vp);
            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
            ImGui::Image(m_pDebugAlbedoBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
        }
        ImGui::End();
    }

    if (m_DebugRoughness)
    {
        if (ImGui::Begin("Roughness", &m_DebugRoughness))
        {
            CD3D11_VIEWPORT vp(0.0f, 0.0f, (float)m_pDebugRoughnessBuffer->GetWidth(), (float)m_pDebugRoughnessBuffer->GetHeight());
            m_DeferredEffect.DebugRoughnessGBuffer(m_pd3dImmediateContext.Get(), m_pDebugRoughnessBuffer->GetRenderTarget(), m_pGBufferSRVs[0], vp);
            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
            ImGui::Image(m_pDebugRoughnessBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
        }
        ImGui::End();
    }

    if (m_DebugMetallic)
    {
        if (ImGui::Begin("Metallic", &m_DebugMetallic))
        {
            CD3D11_VIEWPORT vp(0.0f, 0.0f, (float)m_pDebugMetallicBuffer->GetWidth(), (float)m_pDebugMetallicBuffer->GetHeight());
            m_DeferredEffect.DebugMetallicGBuffer(m_pd3dImmediateContext.Get(), m_pDebugMetallicBuffer->GetRenderTarget(), m_pGBufferSRVs[1], vp);
            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
            ImGui::Image(m_pDebugMetallicBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
        }
        ImGui::End();
    }

    D3D11_VIEWPORT vp = m_pViewerCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &vp);
    ImGui::Render();

    ID3D11RenderTargetView* pRTVs[] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
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
            m_pd3dImmediateContext->OMSetRenderTargets(1, &depthRTV, depthDSV);

            XMMATRIX shadowProj = m_CSManager.GetShadowProjectionXM(cascadeIdx);
            m_ShadowEffect.SetProjMatrix(shadowProj);

            // 更新物体与投影立方体的裁剪
            BoundingOrientedBox obb = m_CSManager.GetShadowOBB(cascadeIdx);
            obb.Transform(obb, m_pLightCamera->GetLocalToWorldMatrixXM());
            m_Sponza.CubeCulling(obb);

            m_Sponza.Draw(m_pd3dImmediateContext.Get(), m_ShadowEffect);

            m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
        }
    }

    m_GpuTimer_Shadow.Stop();
}

void GameApp::RenderGBuffer()
{
    // 注意：我们实际上只需要清空深度缓冲区，因为我们用天空盒的采样来替代没有写入的像素(例如：远平面)
    // 	    这里我们使用深度缓冲区来重建位置且只有在视锥体内的位置会被着色
    // 注意：反转Z的缓冲区，远平面为0
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthBuffer->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

    D3D11_VIEWPORT viewport = m_pViewerCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    BoundingFrustum frustum;
    BoundingFrustum::CreateFromMatrix(frustum, m_pViewerCamera->GetProjMatrixXM());
    frustum.Transform(frustum, m_pViewerCamera->GetLocalToWorldMatrixXM());
    m_Sponza.FrustumCulling(frustum);
    m_Sphere.FrustumCulling(frustum);

    m_GpuTimer_Geometry.Start();
    {
        if (m_EnableNormalMap)
            m_DeferredEffect.SetRenderGBufferWithNormalMap(m_UseTangent, true);
        else
            m_DeferredEffect.SetRenderGBuffer(true);

        m_pd3dImmediateContext->OMSetRenderTargets(static_cast<uint32_t>(m_pGBuffers.size()), m_pGBufferRTVs.data(), m_pDepthBuffer->GetDepthStencil());
        m_Sponza.Draw(m_pd3dImmediateContext.Get(), m_DeferredEffect);
        m_Sphere.Draw(m_pd3dImmediateContext.Get(), m_DeferredEffect);
        m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
    }
    m_GpuTimer_Geometry.Stop();
}


void GameApp::RenderSkybox()
{
    m_GpuTimer_Skybox.Start();
    {
        D3D11_VIEWPORT skyboxViewport = m_pViewerCamera->GetViewPort();
        skyboxViewport.MinDepth = 1.0f;
        skyboxViewport.MaxDepth = 1.0f;
        m_SkyboxToneMapEffect.RenderCubeSkyboxWithToneMap(
            m_pd3dImmediateContext.Get(),
            m_TextureManager.GetTexture("Daylight"),
            m_pDepthBuffer->GetShaderResource(),
            m_pLitBuffer->GetShaderResource(),
            m_pTempBuffer->GetRenderTarget(),
            skyboxViewport,
            m_ToneMapping);
    }
    m_GpuTimer_Skybox.Stop();
}

void GameApp::PostProcess()
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

