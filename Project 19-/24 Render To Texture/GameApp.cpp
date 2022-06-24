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
    // 更新淡入淡出值，并限制摄像机行动
    if (m_FadeUsed)
    {
        m_FadeAmount += m_FadeSign * dt / 2.0f;	// 2s时间淡入/淡出
        if (m_FadeSign > 0.0f && m_FadeAmount > 1.0f)
        {
            m_FadeAmount = 1.0f;
            m_FadeUsed = false;	// 结束淡入
        }
        else if (m_FadeSign < 0.0f && m_FadeAmount < 0.0f)
        {
            m_FadeAmount = 0.0f;
            SendMessage(MainWnd(), WM_DESTROY, 0, 0);	// 关闭程序
            // 这里不结束淡出是因为发送关闭窗口的消息还要过一会才真正关闭
        }
    }
    else
    {
        ImGuiIO& io = ImGui::GetIO();
        // 第一人称的操作
        float d1 = 0.0f, d2 = 0.0f;
        if (ImGui::IsKeyDown(ImGuiKey_W))
            d1 += dt;
        if (ImGui::IsKeyDown(ImGuiKey_S))
            d1 -= dt;
        if (ImGui::IsKeyDown(ImGuiKey_A))
            d2 -= dt;
        if (ImGui::IsKeyDown(ImGuiKey_D))
            d2 += dt;

        m_pCamera->Walk(d1 * 6.0f);
        m_pCamera->Strafe(d2 * 6.0f);

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            m_pCamera->Pitch(io.MouseDelta.y * 0.01f);
            m_pCamera->RotateY(io.MouseDelta.x * 0.01f);
        }

        // 限制移动范围
        XMFLOAT3 adjustedPos;
        XMStoreFloat3(&adjustedPos, XMVectorClamp(m_pCamera->GetPositionXM(), XMVectorReplicate(-90.0f), XMVectorReplicate(90.0f)));
        m_pCamera->SetPosition(adjustedPos);
    }

    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());
    m_PostProcessEffect.SetEyePos(m_pCamera->GetPosition());
    
    if (ImGui::Begin("Render To Texture"))
    {
        if (ImGui::Button("Save as output.jpg"))
        {
            m_PrintScreenStarted = true;
        }

        if (ImGui::Button("Exit & Fade out"))
        {
            m_FadeSign = -1.0f;
            m_FadeUsed = true;
        }
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

    // 绘制主场景
    DrawScene(m_FadeUsed ? m_pLitTexture->GetRenderTarget() : GetBackBufferRTV(),
              m_pDepthTexture->GetDepthStencil(),
              m_pCamera->GetViewPort());

    // 绘制小地图到场景
    CD3D11_VIEWPORT minimapViewport(
        std::max(0.0f, m_ClientWidth - 300.0f), 
        std::max(0.0f, m_ClientHeight - 300.0f), 
        std::min(300.0f, (float)m_ClientWidth),
        std::min(300.0f, (float)m_ClientHeight));
    m_PostProcessEffect.RenderMinimap(
        m_pd3dImmediateContext.Get(),
        m_pMinimapTexture->GetShaderResource(),
        m_FadeUsed ? m_pLitTexture->GetRenderTarget() : GetBackBufferRTV(),
        minimapViewport);

    if (m_FadeUsed)
    {
        // 绘制渐变过程
        m_PostProcessEffect.RenderScreenFade(
            m_pd3dImmediateContext.Get(),
            m_pLitTexture->GetShaderResource(),
            GetBackBufferRTV(),
            m_pCamera->GetViewPort(),
            m_FadeAmount
        );
    }

    // 保存到output.dds和output.png中
    if (m_PrintScreenStarted)
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        GetBackBufferRTV()->GetResource(reinterpret_cast<ID3D11Resource**>(backBuffer.GetAddressOf()));
        // 输出截屏
        HR(SaveWICTextureToFile(m_pd3dImmediateContext.Get(), backBuffer.Get(), GUID_ContainerFormatJpeg, L"output.jpg", &GUID_WICPixelFormat24bppBGR));
        // 结束截屏
        m_PrintScreenStarted = false;
    }

    ID3D11RenderTargetView* pRTVs[] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}



bool GameApp::InitResource()
{

    // ******************
    // 初始化Minimap
    //
    m_pMinimapTexture = std::make_unique<Texture2D>(m_pd3dDevice.Get(), 400, 400, DXGI_FORMAT_R8G8B8A8_UNORM);
    std::unique_ptr<Depth2D> pMinimapDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), 400, 400);
    m_pMinimapTexture->SetDebugObjectName("MinimapTexture");
    pMinimapDepthTexture->SetDebugObjectName("MinimapDepthTexture");
    CD3D11_VIEWPORT minimapViewport(0.0f, 0.0f, 400.0f, 400.0f);

    // ******************
    // 初始化游戏对象
    //

    // 创建随机的树
    CreateRandomTrees();

    // 初始化地面
    Model* pModel = m_ModelManager.CreateFromFile("..\\Model\\ground_24.obj");
    pModel->SetDebugObjectName("Ground");
    m_Ground.SetModel(pModel);
    

    // ******************
    // 初始化摄像机
    //

    // 默认摄像机
    m_pCamera = std::make_unique<FirstPersonCamera>();
    m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    m_pCamera->LookTo(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    // ******************
    // 初始化几乎不会变化的值
    //

    // 黑夜特效
    m_BasicEffect.SetFogColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    m_BasicEffect.SetFogStart(5.0f);
    m_BasicEffect.SetFogRange(20.0f);

    // 小地图范围可视
    m_PostProcessEffect.SetMinimapRect(XMFLOAT4(-95.0f, 95.0f, 95.0f, -95.0f));
    m_PostProcessEffect.SetVisibleRange(25.0f);

    // 方向光(默认)
    DirectionalLight dirLight[4]{};
    dirLight[0].ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
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
    // 渲染小地图纹理
    // 

    m_BasicEffect.SetViewMatrix(XMMatrixLookToLH(
        XMVectorSet(0.0f, 10.0f, 0.0f, 0.0f), 
        XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), 
        XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)));
    // 使用正交投影矩阵(中心在摄像机位置)
    m_BasicEffect.SetProjMatrix(XMMatrixOrthographicLH(190.0f, 190.0f, 1.0f, 20.0f));	
    // 关闭雾效，绘制小地图
    m_BasicEffect.SetFogState(false);
    DrawScene(m_pMinimapTexture->GetRenderTarget(), pMinimapDepthTexture->GetDepthStencil(), minimapViewport);


    // 开启雾效，恢复投影矩阵并设置偏暗的光照
    m_BasicEffect.SetFogState(true);
    m_BasicEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
    dirLight[0].ambient = XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f);
    dirLight[0].diffuse = XMFLOAT4(0.16f, 0.16f, 0.16f, 1.0f);
    for (int i = 0; i < 4; ++i)
        m_BasicEffect.SetDirLight(i, dirLight[i]);
    

    return true;
}

void GameApp::DrawScene(ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV, const D3D11_VIEWPORT& viewport)
{
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pd3dImmediateContext->ClearRenderTargetView(pRTV, black);
    m_pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, pDSV);
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    m_BasicEffect.DrawInstanced(m_pd3dImmediateContext.Get(), *m_pInstancedBuffer, m_Trees, 144);
    // 绘制地面
    m_BasicEffect.SetRenderDefault();
    m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // 清空
    pRTV = nullptr;
    m_pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, nullptr);
}

void GameApp::CreateRandomTrees()
{
    // 初始化树
    Model* pModel = m_ModelManager.CreateFromFile("..\\Model\\tree.obj");
    pModel->SetDebugObjectName("Trees");
    m_Trees.SetModel(pModel);
    XMMATRIX S = XMMatrixScaling(0.015f, 0.015f, 0.015f);

    BoundingBox treeBox = m_Trees.GetModel()->boundingbox;

    // 让树木底部紧贴地面位于y = -2的平面
    treeBox.Transform(treeBox, S);
    float Ty = -(treeBox.Center.y - treeBox.Extents.y + 2.0f);
    // 随机生成144颗随机朝向的树
    std::vector<BasicEffect::InstancedData> treeData(144);
    m_pInstancedBuffer = std::make_unique<Buffer>(m_pd3dDevice.Get(),
        CD3D11_BUFFER_DESC(sizeof(BasicEffect::InstancedData) * 144, D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE));
    m_pInstancedBuffer->SetDebugObjectName("InstancedBuffer");

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real<float> radiusNormDist(0.0f, 30.0f);
    std::uniform_real<float> normDist;
    float theta = 0.0f;
    int pos = 0;
    Transform transform;
    transform.SetScale(0.015f, 0.015f, 0.015f);
    for (int i = 0; i < 16; ++i)
    {
        // 取5-95的半径放置随机的树
        for (int j = 0; j < 3; ++j)
        {
            // 距离越远，树木越多
            for (int k = 0; k < 2 * j + 1; ++k, ++pos)
            {
                float radius = (float)(radiusNormDist(rng) + 30 * j + 5);
                float randomRad = normDist(rng) * XM_2PI / 16;
                transform.SetRotation(0.0f, normDist(rng) * XM_2PI, 0.0f);
                transform.SetPosition(radius * cosf(theta + randomRad), Ty, radius * sinf(theta + randomRad));

                XMStoreFloat4x4(&treeData[pos].world, 
                    XMMatrixTranspose(transform.GetLocalToWorldMatrixXM()));
                XMStoreFloat4x4(&treeData[pos].worldInvTranspose, 
                    XMMatrixTranspose(XMath::InverseTranspose(transform.GetLocalToWorldMatrixXM())));
            }
        }
        theta += XM_2PI / 16;
    }

    memcpy_s(m_pInstancedBuffer->MapDiscard(m_pd3dImmediateContext.Get()), m_pInstancedBuffer->GetByteWidth(),
        treeData.data(), treeData.size() * sizeof(BasicEffect::InstancedData));
    m_pInstancedBuffer->Unmap(m_pd3dImmediateContext.Get());

}
