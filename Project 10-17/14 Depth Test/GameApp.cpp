#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight),
    m_CameraMode(CameraMode::ThirdPerson),
    m_ShadowMat(),
    m_WoodCrateMat()
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;

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

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
    }
}

void GameApp::UpdateScene(float dt)
{
    // 获取子类
    auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);
    auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);
    
    ImGuiIO& io = ImGui::GetIO();
    if (m_CameraMode == CameraMode::Free)
    {
        // 第一人称/自由摄像机的操作
        float d1 = 0.0f, d2 = 0.0f;
        if (ImGui::IsKeyDown(ImGuiKey_W))
            d1 += dt;
        if (ImGui::IsKeyDown(ImGuiKey_S))
            d1 -= dt;
        if (ImGui::IsKeyDown(ImGuiKey_A))
            d2 -= dt;
        if (ImGui::IsKeyDown(ImGuiKey_D))
            d2 += dt;

        cam1st->MoveForward(d1 * 6.0f);
        cam1st->Strafe(d2 * 6.0f);

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            cam1st->Pitch(io.MouseDelta.y * 0.01f);
            cam1st->RotateY(io.MouseDelta.x * 0.01f);
        }
    }
    else if (m_CameraMode == CameraMode::ThirdPerson)
    {
        // 第三人称摄像机的操作
        XMFLOAT3 target = m_BoltAnim.GetTransform().GetPosition();
        cam3rd->SetTarget(target);

        // 绕物体旋转
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            cam3rd->RotateX(io.MouseDelta.y * 0.01f);
            cam3rd->RotateY(io.MouseDelta.x * 0.01f);
        }
        cam3rd->Approach(-io.MouseWheel * 1.0f);
    }

    if (ImGui::Begin("Depth Test"))
    {
        ImGui::Text("W/S/A/D in FPS/Free camera");
        ImGui::Text("Hold the right mouse button and drag the view");

        static int curr_item = 0;
        static const char* modes[] = {
            "Third Person",
            "Free Camera"
        };
        if (ImGui::Combo("Camera Mode", &curr_item, modes, ARRAYSIZE(modes)))
        {
            if (curr_item == 0 && m_CameraMode != CameraMode::ThirdPerson)
            {
                if (!cam3rd)
                {
                    cam3rd = std::make_shared<ThirdPersonCamera>();
                    cam3rd->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
                    m_pCamera = cam3rd;
                }
                XMFLOAT3 target = m_BoltAnim.GetTransform().GetPosition();
                cam3rd->SetTarget(target);
                cam3rd->SetDistance(5.0f);
                cam3rd->SetDistanceMinMax(2.0f, 14.0f);
                cam3rd->SetRotationX(XM_PIDIV4);

                m_CameraMode = CameraMode::ThirdPerson;
            }
            else if (curr_item == 1 && m_CameraMode != CameraMode::Free)
            {
                if (!cam1st)
                {
                    cam1st = std::make_shared<FirstPersonCamera>();
                    cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
                    m_pCamera = cam1st;
                }
                // 从闪电动画上方开始
                XMFLOAT3 pos = m_BoltAnim.GetTransform().GetPosition();
                XMFLOAT3 look{ 0.0f, 0.0f, 1.0f };
                XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
                pos.y += 3;
                cam1st->LookTo(pos, look, up);

                m_CameraMode = CameraMode::Free;
            }
            
        }
    }
    ImGui::End();
    ImGui::Render();

    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
    
    // 更新闪电动画
    static int currBoltFrame = 0;
    static float frameTime = 0.0f;
    m_BoltAnim.SetTexture(mBoltSRVs[currBoltFrame].Get());
    if (frameTime > 1.0f / 60)
    {
        currBoltFrame = (currBoltFrame + 1) % 60;
        frameTime -= 1.0f / 60;
    }
    frameTime += dt;
}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // ******************
    // 1. 给镜面反射区域写入值1到模板缓冲区
    // 

    m_BasicEffect.SetWriteStencilOnly(m_pd3dImmediateContext.Get(), 1);
    m_Mirror.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // ******************
    // 2. 绘制不透明的反射物体
    //

    // 开启反射绘制
    m_BasicEffect.SetReflectionState(true);	// 反射开启
    m_BasicEffect.SetRenderDefaultWithStencil(m_pd3dImmediateContext.Get(), 1);

    m_Walls[2].Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_Walls[3].Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_Walls[4].Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_Floor.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    
    m_WoodCrate.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // ******************
    // 3. 绘制不透明反射物体的阴影
    //

    m_WoodCrate.SetMaterial(m_ShadowMat);
    m_BasicEffect.SetShadowState(true);			// 反射开启，阴影开启
    m_BasicEffect.SetRenderNoDoubleBlend(m_pd3dImmediateContext.Get(), 1);

    m_WoodCrate.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // 恢复到原来的状态
    m_BasicEffect.SetShadowState(false);
    m_WoodCrate.SetMaterial(m_WoodCrateMat);

    // ******************
    // 4. 绘制需要混合的反射闪电动画和透明物体
    //

    m_BasicEffect.SetDrawBoltAnimNoDepthWriteWithStencil(m_pd3dImmediateContext.Get(), 1);
    m_BoltAnim.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    m_BasicEffect.SetReflectionState(false);		// 反射关闭

    m_BasicEffect.SetRenderAlphaBlendWithStencil(m_pd3dImmediateContext.Get(), 1);
    m_Mirror.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    
    // ******************
    // 5. 绘制不透明的正常物体
    //
    m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
    
    for (auto& wall : m_Walls)
        wall.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_Floor.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_WoodCrate.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // ******************
    // 6. 绘制不透明正常物体的阴影
    //
    m_WoodCrate.SetMaterial(m_ShadowMat);
    m_BasicEffect.SetShadowState(true);			// 反射关闭，阴影开启
    m_BasicEffect.SetRenderNoDoubleBlend(m_pd3dImmediateContext.Get(), 0);

    m_WoodCrate.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    m_BasicEffect.SetShadowState(false);			// 阴影关闭
    m_WoodCrate.SetMaterial(m_WoodCrateMat);

    // ******************
    // 7. 绘制需要混合的闪电动画
    m_BasicEffect.SetDrawBoltAnimNoDepthWrite(m_pd3dImmediateContext.Get());
    m_BoltAnim.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
    
    // ******************
    // 初始化游戏对象
    //

    ComPtr<ID3D11ShaderResourceView> texture;
    Material material{};
    material.ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
    material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    material.specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 16.0f);

    m_WoodCrateMat = material;
    m_ShadowMat.ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    m_ShadowMat.diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
    m_ShadowMat.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

    mBoltSRVs.assign(60, nullptr);
    wchar_t wstr[50];
    // 初始化闪电
    for (int i = 1; i <= 60; ++i)
    {
        wsprintf(wstr, L"..\\Texture\\BoltAnim\\Bolt%03d.bmp", i);
        HR(CreateWICTextureFromFile(m_pd3dDevice.Get(), wstr, nullptr, mBoltSRVs[static_cast<size_t>(i) - 1].GetAddressOf()));
    }

    m_BoltAnim.SetBuffer(m_pd3dDevice.Get(), Geometry::CreateCylinderNoCap(4.0f, 4.0f));
    // 抬起高度避免深度缓冲区资源争夺
    m_BoltAnim.GetTransform().SetPosition(0.0f, 2.01f, 0.0f);
    m_BoltAnim.SetMaterial(material);
    
    // 初始化木盒
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\WoodCrate.dds", nullptr, texture.GetAddressOf()));
    m_WoodCrate.SetBuffer(m_pd3dDevice.Get(), Geometry::CreateBox());
    // 抬起高度避免深度缓冲区资源争夺
    m_WoodCrate.GetTransform().SetPosition(0.0f, 0.01f, 0.0f);
    m_WoodCrate.SetTexture(texture.Get());
    m_WoodCrate.SetMaterial(material);
    

    // 初始化地板
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\floor.dds", nullptr, texture.ReleaseAndGetAddressOf()));
    m_Floor.SetBuffer(m_pd3dDevice.Get(),
        Geometry::CreatePlane(XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
    m_Floor.SetTexture(texture.Get());
    m_Floor.SetMaterial(material);
    m_Floor.GetTransform().SetPosition(0.0f, -1.0f, 0.0f);

    // 初始化墙体
    m_Walls.resize(5);
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\brick.dds", nullptr, texture.ReleaseAndGetAddressOf()));
    // 这里控制墙体五个面的生成，0和1的中间位置用于放置镜面
    //     ____     ____
    //    /| 0 |   | 1 |\
    //   /4|___|___|___|2\
    //  /_/_ _ _ _ _ _ _\_\
    // | /       3       \ |
    // |/_________________\|
    //
    for (int i = 0; i < 5; ++i)
    {
        m_Walls[i].SetMaterial(material);
        m_Walls[i].SetTexture(texture.Get());
    }
    m_Walls[0].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
    m_Walls[1].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
    m_Walls[2].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
    m_Walls[3].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
    m_Walls[4].SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
    
    m_Walls[0].GetTransform().SetRotation(-XM_PIDIV2, 0.0f, 0.0f);
    m_Walls[0].GetTransform().SetPosition(-7.0f, 3.0f, 10.0f);
    m_Walls[1].GetTransform().SetRotation(-XM_PIDIV2, 0.0f, 0.0f);
    m_Walls[1].GetTransform().SetPosition(7.0f, 3.0f, 10.0f);
    m_Walls[2].GetTransform().SetRotation(-XM_PIDIV2, XM_PIDIV2, 0.0f);
    m_Walls[2].GetTransform().SetPosition(10.0f, 3.0f, 0.0f);
    m_Walls[3].GetTransform().SetRotation(-XM_PIDIV2, XM_PI, 0.0f);
    m_Walls[3].GetTransform().SetPosition(0.0f, 3.0f, -10.0f);
    m_Walls[4].GetTransform().SetRotation(-XM_PIDIV2, -XM_PIDIV2, 0.0f);
    m_Walls[4].GetTransform().SetPosition(-10.0f, 3.0f, 0.0f);

    // 初始化镜面
    material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
    material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\ice.dds", nullptr, texture.ReleaseAndGetAddressOf()));
    m_Mirror.SetBuffer(m_pd3dDevice.Get(),
        Geometry::CreatePlane(XMFLOAT2(8.0f, 8.0f), XMFLOAT2(1.0f, 1.0f)));
    m_Mirror.GetTransform().SetRotation(-XM_PIDIV2, 0.0f, 0.0f);
    m_Mirror.GetTransform().SetPosition(0.0f, 3.0f, 10.0f);
    m_Mirror.SetTexture(texture.Get());
    m_Mirror.SetMaterial(material);

    // ******************
    // 初始化摄像机
    //
    auto camera = std::make_shared<ThirdPersonCamera>();
    m_pCamera = camera;
    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    camera->SetTarget(m_BoltAnim.GetTransform().GetPosition());
    camera->SetDistance(5.0f);
    camera->SetDistanceMinMax(2.0f, 14.0f);
    camera->SetRotationX(XM_PIDIV2);

    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

    m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
    m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());

    // ******************
    // 初始化不会变化的值
    //
    m_BasicEffect.SetReflectionMatrix(XMMatrixReflect(XMVectorSet(0.0f, 0.0f, -1.0f, 10.0f)));
    // 稍微高一点位置以显示阴影
    m_BasicEffect.SetShadowMatrix(XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.99f), XMVectorSet(0.0f, 10.0f, -10.0f, 1.0f)));
    m_BasicEffect.SetRefShadowMatrix(XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.99f), XMVectorSet(0.0f, 10.0f, 30.0f, 1.0f)));
    // 环境光
    DirectionalLight dirLight;
    dirLight.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    dirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight.direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
    m_BasicEffect.SetDirLight(0, dirLight);
    // 灯光
    PointLight pointLight;
    pointLight.position = XMFLOAT3(0.0f, 10.0f, -10.0f);
    pointLight.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    pointLight.diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
    pointLight.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    pointLight.att = XMFLOAT3(0.0f, 0.1f, 0.0f);
    pointLight.range = 25.0f;
    m_BasicEffect.SetPointLight(0, pointLight);
    
    // ******************
    // 设置调试对象名
    //
    m_BoltAnim.SetDebugObjectName("BoltAnim");
    m_Floor.SetDebugObjectName("Floor");
    m_Mirror.SetDebugObjectName("Mirror");
    m_Walls[0].SetDebugObjectName("Walls[0]");
    m_Walls[1].SetDebugObjectName("Walls[1]");
    m_Walls[2].SetDebugObjectName("Walls[2]");
    m_Walls[3].SetDebugObjectName("Walls[3]");
    m_Walls[4].SetDebugObjectName("Walls[4]");
    m_WoodCrate.SetDebugObjectName("WoodCrate");

    return true;
}



