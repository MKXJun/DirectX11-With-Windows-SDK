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
    m_pDepthTexture->SetDebugObjectName("DepthTexture");

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
    // ******************
    // 记录并更新物体位置和旋转弧度
    //
    static float theta = 0.0f, phi = 0.0f;
    static XMMATRIX Left = XMMatrixTranslation(-5.0f, 0.0f, 0.0f);
    static XMMATRIX Top = XMMatrixTranslation(0.0f, 4.0f, 0.0f);
    static XMMATRIX Right = XMMatrixTranslation(5.0f, 0.0f, 0.0f);
    static XMMATRIX Bottom = XMMatrixTranslation(0.0f, -4.0f, 0.0f);

    theta += dt * 0.5f;
    phi += dt * 0.3f;
    // 更新物体运动
    m_Sphere.GetTransform().SetPosition(-5.0f, 0.0f, 0.0f);
    m_Cube.GetTransform().SetPosition(0.0f, 4.0f, 0.0f);
    m_Cube.GetTransform().SetRotation(-phi, theta, 0.0f);
    m_Cylinder.GetTransform().SetPosition(5.0f, 0.0f, 0.0f);
    m_Cylinder.GetTransform().SetRotation(phi, theta, 0.0f);
    m_House.GetTransform().SetPosition(0.0f, -4.0f, 0.0f);
    m_House.GetTransform().SetRotation(0.0f, theta, 0.0f);
    m_House.GetTransform().SetScale(0.005f, 0.005f, 0.005f);
    m_Triangle.GetTransform().SetRotation(0.0f, theta, 0.0f);

    ImGuiIO& io = ImGui::GetIO();
    // ******************
    // 拾取检测
    //
    ImVec2 mousePos = ImGui::GetMousePos();
    mousePos.x = std::clamp(mousePos.x, 0.0f, m_ClientWidth - 1.0f);
    mousePos.y = std::clamp(mousePos.y, 0.0f, m_ClientHeight - 1.0f);
    Ray ray = Ray::ScreenToRay(*m_pCamera, mousePos.x, mousePos.y);
    
    // 三角形顶点变换
    static XMVECTOR V[3];
    for (int i = 0; i < 3; ++i)
    {
        V[i] = XMVector3TransformCoord(XMLoadFloat3(&m_TriangleMesh.vertices[i]), 
            XMMatrixRotationY(theta));
    }

    bool hitObject = false;
    std::string pickedObjStr = "None";
    if (ray.Hit(m_BoundingSphere))
    {
        pickedObjStr = "Sphere";
        hitObject = true;
    }
    else if (ray.Hit(m_Cube.GetBoundingOrientedBox()))
    {
        pickedObjStr = "Cube";
        hitObject = true;
    }
    else if (ray.Hit(m_Cylinder.GetBoundingOrientedBox()))
    {
        pickedObjStr = "Cylinder";
        hitObject = true;
    }
    else if (ray.Hit(m_House.GetBoundingOrientedBox()))
    {
        pickedObjStr = "House";
        hitObject = true;
    }
    else if (ray.Hit(V[0], V[1], V[2]))
    {
        pickedObjStr = "Triangle";
        hitObject = true;
    }

    if (hitObject == true && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        std::wstring wstr = L"You clicked ";
        wstr += UTF8ToWString(pickedObjStr) + L"!";
        MessageBox(nullptr, wstr.c_str(), L"Message", 0);
    }

    if (ImGui::Begin("Picking"))
    {
        ImGui::Text("Current Object: %s", pickedObjStr.c_str());
    }
    ImGui::End();
    ImGui::Render();
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

    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pd3dImmediateContext->ClearRenderTargetView(GetBackBufferRTV(), black);
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ID3D11RenderTargetView* pRTVs[1] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
    D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    m_BasicEffect.SetRenderDefault();

    // 绘制不需要纹理的模型
    m_Sphere.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_Cube.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_Cylinder.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_Triangle.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // 绘制需要纹理的模型
    m_House.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}

bool GameApp::InitResource()
{
    // ******************
    // 初始化游戏对象
    //
    
    // 球体(预先设好包围球)
    Model* pModel = m_ModelManager.CreateFromGeometry("Sphere", Geometry::CreateSphere());
    m_Sphere.SetModel(pModel);
    pModel->SetDebugObjectName("Sphere");
    m_BoundingSphere.Center = XMFLOAT3(-5.0f, 0.0f, 0.0f);
    m_BoundingSphere.Radius = 1.0f;
    // 立方体
    pModel = m_ModelManager.CreateFromGeometry("Cube", Geometry::CreateBox());
    m_Cube.SetModel(pModel);
    pModel->SetDebugObjectName("Cube");
    // 圆柱体
    pModel = m_ModelManager.CreateFromGeometry("Cylinder", Geometry::CreateCylinder());
    m_Cylinder.SetModel(pModel);
    pModel->SetDebugObjectName("Cylinder");
    // 房屋
    pModel = m_ModelManager.CreateFromFile("..\\Model\\house.obj");
    m_House.SetModel(pModel);
    pModel->SetDebugObjectName("House");

    // 三角形(带反面)
    m_TriangleMesh.vertices.assign({ 
        XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, -1.0f, 0.0f),
        XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(-1.0f, -1.0f, 0.0f) 
        });
    m_TriangleMesh.normals.assign({
        XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),
        XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)
        });
    m_TriangleMesh.texcoords.assign(6, XMFLOAT2());
    m_TriangleMesh.indices16.assign({ 0, 1, 2, 3, 4, 5 });
    pModel = m_ModelManager.CreateFromGeometry("Triangle", m_TriangleMesh);
    m_Triangle.SetModel(pModel);
    pModel->SetDebugObjectName("Triangle");

    // ******************
    // 初始化摄像机
    //
    auto camera = std::make_shared<FirstPersonCamera>();
    m_pCamera = camera;
    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    camera->LookTo(XMFLOAT3(0.0f, 0.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    m_BasicEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_BasicEffect.SetProjMatrix(camera->GetProjMatrixXM());
    

    // ******************
    // 初始化不会变化的值
    //

    // 方向光
    DirectionalLight dirLight;
    dirLight.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    dirLight.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
    dirLight.direction = XMFLOAT3(-0.707f, -0.707f, 0.707f);
    m_BasicEffect.SetDirLight(0, dirLight);

    // 默认只按对象绘制
    m_BasicEffect.SetRenderDefault();

    return true;
}

