#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight),
    m_GroundMode(GroundMode::Floor),
    m_EnableNormalMap(true),
    m_SphereRad()
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

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{
    D3DApp::OnResize();

    m_pDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
    m_pDepthTexture->SetDebugObjectName("DepthTexture");

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_BasicEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_SkyboxEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
    }
}

void GameApp::UpdateScene(float dt)
{

    ImVec2 mousePos = ImGui::GetMousePos();
    ImGuiIO& io = ImGui::GetIO();
    static int mouseStatus = 0;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        if (mousePos.x >= m_DebugTextureXY.x && mousePos.x < m_DebugTextureXY.x + m_DebugTextureWH.x &&
            mousePos.y >= m_DebugTextureXY.y && mousePos.y < m_DebugTextureXY.y + m_DebugTextureWH.y)
            mouseStatus = 1;
        else
            mouseStatus = 0;
    }

    if (mouseStatus == 1)
    {
        float yaw = 0.0f, pitch = 0.0f;
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {

            yaw += io.MouseDelta.x * 0.015f;
            pitch += io.MouseDelta.y * 0.015f;
        }
        m_pDebugCamera->RotateY(yaw);
        m_pDebugCamera->Pitch(pitch);
    }
    else
    {
        m_CameraController.Update(dt);
    }

    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

    if (ImGui::Begin("Normal Mapping"))
    {
        ImGui::Checkbox("Enable Normalmap", &m_EnableNormalMap);

        static const char* ground_strs[] = {
            "Floor",
            "Stones"
        };
        static int ground_item = static_cast<int>(m_GroundMode);
        if (ImGui::Combo("Ground Mode", &ground_item, ground_strs, ARRAYSIZE(ground_strs)))
        {
            Model* pModel = m_ModelManager.GetModel("Ground");
            switch (ground_item)
            {
            case 0: 
                pModel->materials[0].Set<std::string>("$Diffuse", "floor");
                pModel->materials[0].Set<std::string>("$Normal", "floorN");
                break;
            case 1: 
                pModel->materials[0].Set<std::string>("$Diffuse", "stones");
                pModel->materials[0].Set<std::string>("$Normal", "stonesN");
                break;
            default:
                break;
            }
            
        }
    }
    ImGui::End();

    // 设置球体动画
    m_SphereRad += 2.0f * dt;
    for (size_t i = 0; i < 4; ++i)
    {
        auto& transform = m_Spheres[i].GetTransform();
        auto pos = transform.GetPosition();
        pos.y = 0.5f * std::sin(m_SphereRad);
        transform.SetPosition(pos);
    }
    m_Spheres[4].GetTransform().RotateAround(XMFLOAT3(), XMFLOAT3(0.0f, 1.0f, 0.0f), 2.0f * dt);
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

    // ******************
    // 生成动态天空盒
    //

    static XMFLOAT3 ups[6] = {
        { 0.0f, 1.0f, 0.0f },	// +X
        { 0.0f, 1.0f, 0.0f },   // -X
        { 0.0f, 0.0f, -1.0f },  // +Y
        { 0.0f, 0.0f, 1.0f },   // -Y
        { 0.0f, 1.0f, 0.0f },   // +Z
        { 0.0f, 1.0f, 0.0f }    // -Z
    };

    static XMFLOAT3 looks[6] = {
        { 1.0f, 0.0f, 0.0f },	// +X
        { -1.0f, 0.0f, 0.0f },  // -X
        { 0.0f, 1.0f, 0.0f },	// +Y
        { 0.0f, -1.0f, 0.0f },  // -Y
        { 0.0f, 0.0f, 1.0f },	// +Z
        { 0.0f, 0.0f, -1.0f },  // -Z
    };

    BoundingFrustum frustum;
    BoundingFrustum::CreateFromMatrix(frustum, m_pCamera->GetProjMatrixXM());
    frustum.Transform(frustum, m_pCamera->GetLocalToWorldMatrixXM());

    // 中心球绘制量较大，能不画就不画
    m_CenterSphere.FrustumCulling(frustum);
    m_BasicEffect.SetEyePos(m_CenterSphere.GetTransform().GetPosition());
    if (m_CenterSphere.InFrustum())
    {
        // 绘制动态天空盒的每个面（以球体为中心）
        for (int i = 0; i < 6; ++i)
        {
            m_pCubeCamera->LookTo(m_CenterSphere.GetTransform().GetPosition(), looks[i], ups[i]);

            // 不绘制中心球
            DrawScene(false, *m_pCubeCamera, m_pDynamicTextureCube->GetRenderTarget(i), m_pDynamicCubeDepthTexture->GetDepthStencil());
        }
    }

    // ******************
    // 绘制场景
    //

    DrawScene(true, *m_pCamera, GetBackBufferRTV(), m_pDepthTexture->GetDepthStencil());

    // 绘制天空盒
    static bool debugCube = false;
    if (ImGui::Begin("Normal Mapping"))
    {
        ImGui::Checkbox("Debug Cube", &debugCube);
    }
    ImGui::End();

    m_DebugTextureXY = {};
    m_DebugTextureWH = {};
    if (debugCube)
    {
        if (ImGui::Begin("Debug"))
        {
            D3D11_VIEWPORT viewport = m_pDebugCamera->GetViewPort();
            ID3D11RenderTargetView* pRTVs[]{ m_pDebugDynamicCubeTexture->GetRenderTarget() };
            m_pd3dImmediateContext->RSSetViewports(1, &viewport);
            m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
            m_SkyboxEffect.SetRenderDefault();
            m_SkyboxEffect.SetViewMatrix(m_pDebugCamera->GetViewMatrixXM());
            m_SkyboxEffect.SetProjMatrix(m_pDebugCamera->GetProjMatrixXM());
            m_DebugSkybox.Draw(m_pd3dImmediateContext.Get(), m_SkyboxEffect);
            // 画完后清空
            ID3D11ShaderResourceView* nullSRVs[3]{};
            m_pd3dImmediateContext->PSSetShaderResources(0, 3, nullSRVs);
            // 复原
            viewport = m_pCamera->GetViewPort();
            pRTVs[0] = GetBackBufferRTV();
            m_pd3dImmediateContext->RSSetViewports(1, &viewport);
            m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);

            ImVec2 winSize = ImGui::GetWindowSize();
            float smaller = (std::min)(winSize.x - 20, winSize.y - 36);
            ImGui::Image(m_pDebugDynamicCubeTexture->GetShaderResource(), ImVec2(smaller, smaller));
            m_DebugTextureXY = ImGui::GetItemRectMin();
            m_DebugTextureWH = { smaller, smaller };
        }
        ImGui::End();
    }
    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}



bool GameApp::InitResource()
{
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
            pCubeTextures.push_back(m_TextureManager.CreateFromFile(filenameStr, false, true));
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

    // 动态天空盒
    m_pDynamicTextureCube = std::make_unique<TextureCube>(m_pd3dDevice.Get(), 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDynamicCubeDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), 256, 256);
    m_pDebugDynamicCubeTexture = std::make_unique<Texture2D>(m_pd3dDevice.Get(), 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_TextureManager.AddTexture("DynamicCube", m_pDynamicTextureCube->GetShaderResource());

    m_pDynamicTextureCube->SetDebugObjectName("DynamicTextureCube");
    m_pDynamicCubeDepthTexture->SetDebugObjectName("DynamicCubeDepthTexture");
    m_pDebugDynamicCubeTexture->SetDebugObjectName("DebugDynamicCubeTexture");

    // ******************
    // 初始化游戏对象
    //
    // 球体
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Sphere", Geometry::CreateSphere());
        pModel->SetDebugObjectName("Sphere");
        m_TextureManager.CreateFromFile("..\\Texture\\stone.dds", false, true);
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\stone.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        pModel->materials[0].Set<XMFLOAT4>("$ReflectColor", XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));

        Transform sphereTransforms[] = {
        Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(4.5f, 0.0f, 4.5f)),
        Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(-4.5f, 0.0f, 4.5f)),
        Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(-4.5f, 0.0f, -4.5f)),
        Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(4.5f, 0.0f, -4.5f)),
        Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(2.5f, 0.0f, 0.0f)),
        };

        for (size_t i = 0; i < 5; ++i)
        {
            m_Spheres[i].GetTransform() = sphereTransforms[i];
            m_Spheres[i].SetModel(pModel);
        }
        m_CenterSphere.SetModel(pModel);
    }
    // 地面
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Ground", Geometry::CreatePlane(XMFLOAT2(10.0f, 10.0f), XMFLOAT2(5.0f, 5.0f)));
        pModel->SetDebugObjectName("Ground");
        m_TextureManager.AddTexture("floor", m_TextureManager.CreateFromFile("..\\Texture\\floor.dds"));
        m_TextureManager.AddTexture("floorN", m_TextureManager.CreateFromFile("..\\Texture\\floor_nmap.dds"));
        m_TextureManager.AddTexture("stones", m_TextureManager.CreateFromFile("..\\Texture\\stones.dds"));
        m_TextureManager.AddTexture("stonesN", m_TextureManager.CreateFromFile("..\\Texture\\stones_nmap.dds"));
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\floor.dds");
        pModel->materials[0].Set<std::string>("$Normal", "..\\Texture\\floor_nmap.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        pModel->materials[0].Set<XMFLOAT4>("$ReflectColor", XMFLOAT4());
        m_Ground.SetModel(pModel);
        m_Ground.GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
    }
    // 柱体
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Cylinder", Geometry::CreateCylinder(0.5f, 2.0f));
        pModel->SetDebugObjectName("Cylinder");
        m_TextureManager.CreateFromFile("..\\Texture\\bricks.dds");
        m_TextureManager.CreateFromFile("..\\Texture\\bricks_nmap.dds");
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\bricks.dds");
        pModel->materials[0].Set<std::string>("$Normal", "..\\Texture\\bricks_nmap.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        pModel->materials[0].Set<XMFLOAT4>("$ReflectColor", XMFLOAT4());

        // 需要固定位置
        Transform cylinderTransforms[] = {
            Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(0.0f, -1.99f, 0.0f)),
            Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(4.5f, -1.99f, 4.5f)),
            Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(-4.5f, -1.99f, 4.5f)),
            Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(-4.5f, -1.99f, -4.5f)),
            Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(4.5f, -1.99f, -4.5f)),
        };

        for (size_t i = 0; i < 5; ++i)
        {
            m_Cylinders[i].SetModel(pModel);
            m_Cylinders[i].GetTransform() = cylinderTransforms[i];
        }
    }
    // 天空盒立方体
    Model* pModel = m_ModelManager.CreateFromGeometry("Skybox", Geometry::CreateBox());
    pModel->SetDebugObjectName("Skybox");
    pModel->materials[0].Set<std::string>("$Skybox", "Daylight");
    m_Skybox.SetModel(pModel);

    // 调试用立方体
    pModel = m_ModelManager.CreateFromGeometry("DebugSkybox", Geometry::CreateBox());
    pModel->SetDebugObjectName("DebugSkybox");
    pModel->materials[0].Set<std::string>("$Skybox", "DynamicCube");
    m_DebugSkybox.SetModel(pModel);

    // ******************
    // 初始化摄像机
    //
    m_pCamera = std::make_shared<FirstPersonCamera>();
    m_CameraController.InitCamera(m_pCamera.get());
    m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    m_pCamera->LookTo(XMFLOAT3(0.0f, 0.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    m_pCubeCamera = std::make_shared<FirstPersonCamera>();
    m_pCubeCamera->SetFrustum(XM_PIDIV2, 1.0f, 0.1f, 1000.0f);
    m_pCubeCamera->SetViewPort(0.0f, 0.0f, 256.0f, 256.0f);

    m_pDebugCamera = std::make_shared<FirstPersonCamera>();
    m_pDebugCamera->SetFrustum(XM_PIDIV2, 1.0f, 0.1f, 1000.0f);
    m_pDebugCamera->SetViewPort(0.0f, 0.0f, 256.0f, 256.0f);

    // ******************
    // 初始化不会变化的值
    //

    // 方向光
    DirectionalLight dirLight[4]{};
    dirLight[0].ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
    dirLight[0].diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight[0].specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
    dirLight[0].direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
    dirLight[1] = dirLight[0];
    dirLight[1].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
    dirLight[2] = dirLight[0];
    dirLight[2].direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
    dirLight[3] = dirLight[0];
    dirLight[3].direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
    for (int i = 0; i < 4; ++i)
        m_BasicEffect.SetDirLight(i, dirLight[i]);

    return true;
}

void GameApp::DrawScene(bool drawCenterSphere, const Camera& camera, ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV)
{
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pd3dImmediateContext->ClearRenderTargetView(pRTV, black);
    m_pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, pDSV);
    
    BoundingFrustum frustum;
    BoundingFrustum::CreateFromMatrix(frustum, camera.GetProjMatrixXM());
    frustum.Transform(frustum, camera.GetLocalToWorldMatrixXM());
    D3D11_VIEWPORT viewport = camera.GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);
    // 绘制模型
    m_BasicEffect.SetViewMatrix(camera.GetViewMatrixXM());
    m_BasicEffect.SetProjMatrix(camera.GetProjMatrixXM());
    m_BasicEffect.SetEyePos(camera.GetPosition());
    m_BasicEffect.SetRenderDefault();

    // 只绘制球体的反射效果
    if (drawCenterSphere)
    {
        m_BasicEffect.SetReflectionEnabled(true);
        m_BasicEffect.SetRefractionEnabled(false);
        m_BasicEffect.SetTextureCube(m_pDynamicTextureCube->GetShaderResource());
        m_CenterSphere.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
        m_BasicEffect.SetTextureCube(nullptr);
    }
    
    
    m_BasicEffect.SetReflectionEnabled(false);
    m_BasicEffect.SetRefractionEnabled(false);
    
    if (m_EnableNormalMap)
        m_BasicEffect.SetRenderWithNormalMap();
    else
        m_BasicEffect.SetRenderDefault();

    // 绘制地面
    m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);


    // 绘制五个圆柱
    for (auto& cylinder : m_Cylinders)
    {
        cylinder.FrustumCulling(frustum);
        cylinder.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    }

    m_BasicEffect.SetRenderDefault();
    // 绘制五个圆球
    for (auto& sphere : m_Spheres)
    {
        sphere.FrustumCulling(frustum);
        sphere.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    }

    // 绘制天空盒
    m_SkyboxEffect.SetViewMatrix(camera.GetViewMatrixXM());
    m_SkyboxEffect.SetProjMatrix(camera.GetProjMatrixXM());
    m_SkyboxEffect.SetRenderDefault();
    m_Skybox.Draw(m_pd3dImmediateContext.Get(), m_SkyboxEffect);
    
}

