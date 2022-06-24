#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight),
    m_CameraMode(CameraMode::Free),
    m_EnableAlphaToCoverage(true),
    m_FogEnabled(true),
    m_FogRange(75.0f),
    m_IsNight(false),
    m_TreeMat()
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

    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
    }

}

void GameApp::UpdateScene(float dt)
{
    // 获取子类
    auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

    ImGuiIO& io = ImGui::GetIO();
    // ******************
    // 自由摄像机的操作
    // 
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

    // 将位置限制在[-49.9f, 49.9f]的区域内
    // 不允许穿地
    XMFLOAT3 adjustedPos;
    XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(),
        XMVectorSet(-49.9f, 0.0f, -49.9f, 0.0f), XMVectorSet(49.9f, 99.9f, 49.9f, 0.0f)));
    cam1st->SetPosition(adjustedPos);

    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());
    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        cam1st->Pitch(io.MouseDelta.y * 0.01f);
        cam1st->RotateY(io.MouseDelta.x * 0.01f);
    }

    if (ImGui::Begin("Tree Billboard"))
    {
        static int curr_item = 0;
        static const char* modes[] = {
            "Daytime",
            "Dark Night",
        };
        ImGui::Checkbox("Enable Alpha-To-Coverage", &m_EnableAlphaToCoverage);
        if (ImGui::Checkbox("Enable Fog", &m_FogEnabled))
        {
            m_BasicEffect.SetFogState(m_FogEnabled);
        }

        if (m_FogEnabled)
        {
            if (ImGui::Combo("Fog Mode", &curr_item, modes, ARRAYSIZE(modes)))
            {
                m_IsNight = (curr_item == 1);
                if (m_IsNight)
                {
                    // 黑夜模式下变为逐渐黑暗
                    m_BasicEffect.SetFogColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
                    m_BasicEffect.SetFogStart(5.0f);
                }
                else
                {
                    // 白天模式则对应雾效
                    m_BasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
                    m_BasicEffect.SetFogStart(15.0f);
                }
            }
            if (ImGui::SliderFloat("Fog Range", &m_FogRange, 15.0f, 175.0f, "%.0f"))
            {
                m_BasicEffect.SetFogRange(m_FogRange);
            }
            float fog_start = m_IsNight ? 5.0f : 15.0f;
            ImGui::Text("Fog: %.0f-%.0f", fog_start, m_FogRange + fog_start);
        }
    }
    ImGui::End();
    ImGui::Render();

}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    // ******************
    // 绘制Direct3D部分
    //
    
    // 设置背景色
    if (m_IsNight)
    {
        m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
    }
    else
    {
        m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
    }
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // 绘制地面
    m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
    m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // 绘制树
    m_BasicEffect.SetRenderBillboard(m_pd3dImmediateContext.Get(), m_EnableAlphaToCoverage);
    m_BasicEffect.SetMaterial(m_TreeMat);
    UINT stride = sizeof(VertexPosSize);
    UINT offset = 0;
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, mPointSpritesBuffer.GetAddressOf(), &stride, &offset);
    m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
    m_pd3dImmediateContext->Draw(16, 0);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
    // ******************
    // 初始化各种物体
    //

    // 初始化树纹理资源
    HR(CreateTexture2DArrayFromFile(
        m_pd3dDevice.Get(),
        m_pd3dImmediateContext.Get(),
        std::vector<std::wstring>{
            L"..\\Texture\\tree0.dds",
            L"..\\Texture\\tree1.dds",
            L"..\\Texture\\tree2.dds",
            L"..\\Texture\\tree3.dds"},
        nullptr,
        mTreeTexArray.GetAddressOf()));
    m_BasicEffect.SetTextureArray(mTreeTexArray.Get());

    // 初始化点精灵缓冲区
    InitPointSpritesBuffer();

    // 初始化树的材质
    m_TreeMat.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_TreeMat.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_TreeMat.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

    ComPtr<ID3D11ShaderResourceView> texture;
    // 初始化地板
    m_Ground.SetBuffer(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(100.0f, 100.0f), XMFLOAT2(10.0f, 10.0f)));
    m_Ground.GetTransform().SetPosition(0.0f, -5.0f, 0.0f);
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\Grass.dds", nullptr, texture.GetAddressOf()));
    m_Ground.SetTexture(texture.Get());
    Material material{};
    material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
    m_Ground.SetMaterial(material);

    // ******************
    // 初始化不会变化的值
    //

    // 方向光
    DirectionalLight dirLight[4];
    dirLight[0].ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
    dirLight[0].diffuse = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
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

    // ******************
    // 初始化摄像机
    //
    auto camera = std::make_shared<FirstPersonCamera>();
    m_pCamera = camera;
    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    camera->SetPosition(XMFLOAT3());
    camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    camera->LookTo(XMFLOAT3(), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
    m_BasicEffect.SetViewMatrix(camera->GetViewXM());
    m_BasicEffect.SetProjMatrix(camera->GetProjXM());
    m_BasicEffect.SetEyePos(camera->GetPosition());

    // ******************
    // 初始化雾效和天气等
    //

    m_BasicEffect.SetFogState(m_FogEnabled);
    m_BasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
    m_BasicEffect.SetFogStart(15.0f);
    m_BasicEffect.SetFogRange(75.0f);

    // ******************
    // 设置调试对象名
    //
    m_Ground.SetDebugObjectName("Ground");
    D3D11SetDebugObjectName(mPointSpritesBuffer.Get(), "PointSpritesVertexBuffer");
    D3D11SetDebugObjectName(mTreeTexArray.Get(), "TreeTexArray");
    
    return true;
}

void GameApp::InitPointSpritesBuffer()
{
    srand((unsigned)time(nullptr));
    VertexPosSize vertexes[16];
    float theta = 0.0f;
    for (int i = 0; i < 16; ++i)
    {
        // 取20-50的半径放置随机的树
        float radius = (float)(rand() % 31 + 20);
        float randomRad = rand() % 256 / 256.0f * XM_2PI / 16;
        vertexes[i].pos = XMFLOAT3(radius * cosf(theta + randomRad), 8.0f, radius * sinf(theta + randomRad));
        vertexes[i].size = XMFLOAT2(30.0f, 30.0f);
        theta += XM_2PI / 16;
    }

    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;	// 数据不可修改
    vbd.ByteWidth = sizeof (vertexes);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertexes;
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, mPointSpritesBuffer.GetAddressOf()));
}
