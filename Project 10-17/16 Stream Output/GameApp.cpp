#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight),
    m_ShowMode(Mode::SplitedTriangle),
    m_CurrIndex(),
    m_IsWireFrame(false),
    m_ShowNormal(false),
    m_InitVertexCounts()
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

    // 更新投影矩阵
    m_BasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));

}

void GameApp::UpdateScene(float dt)
{
    UINT stride = (m_ShowMode != Mode::SplitedSphere ? sizeof(VertexPosColor) : sizeof(VertexPosNormalColor));
    UINT offset = 0;

    if (ImGui::Begin("Stream Output"))
    {
        static int curr_item = 0;
        static const char* modes[] = {
            "Splited Triangle",
            "Splited Snow",
            "Splited Sphere"
        };
        if (ImGui::Combo("Mode", &curr_item, modes, ARRAYSIZE(modes)))
        {
            m_ShowMode = static_cast<Mode>(curr_item);
            m_IsWireFrame = false;
            m_ShowNormal = false;
            m_CurrIndex = 0;
            switch (m_ShowMode)
            {
            case GameApp::Mode::SplitedTriangle:
                ResetSplitedTriangle();
                stride = sizeof(VertexPosColor);
                break;
            case GameApp::Mode::SplitedSnow:
                ResetSplitedSnow();
                m_IsWireFrame = true;
                stride = sizeof(VertexPosColor);
                break;
            case GameApp::Mode::SplitedSphere:
                ResetSplitedSphere();
                stride = sizeof(VertexPosNormalColor);
                break;
            default:
                break;
            }
            m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[m_CurrIndex].GetAddressOf(), &stride, &offset);
        }

        if (ImGui::SliderInt("Level", &m_CurrIndex, 0, 6))
            m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[m_CurrIndex].GetAddressOf(), &stride, &offset);

        if (m_ShowMode != Mode::SplitedSnow)
            ImGui::Checkbox("Show Wireframe", &m_IsWireFrame);

        if (m_ShowMode == Mode::SplitedSphere)
            ImGui::Checkbox("Show Normal", &m_ShowNormal);
    }
    ImGui::End();
    ImGui::Render();


    // ******************
    // 更新每帧变化的值
    //
    if (m_ShowMode == Mode::SplitedSphere)
    {
        // 让球体转起来
        static float theta = 0.0f;
        theta += 0.3f * dt;
        m_BasicEffect.SetWorldMatrix(XMMatrixRotationY(theta));
    }
    else
    {
        m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
    }

}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);


    m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


    // 根据当前绘制模式设置需要用于渲染的各项资源
    if (m_ShowMode == Mode::SplitedTriangle)
    {
        m_BasicEffect.SetRenderSplitedTriangle(m_pd3dImmediateContext.Get());
    }
    else if (m_ShowMode == Mode::SplitedSnow)
    {
        m_BasicEffect.SetRenderSplitedSnow(m_pd3dImmediateContext.Get());
    }
    else if (m_ShowMode == Mode::SplitedSphere)
    {
        m_BasicEffect.SetRenderSplitedSphere(m_pd3dImmediateContext.Get());
    }

    // 设置线框/面模式
    if (m_IsWireFrame)
    {
        m_pd3dImmediateContext->RSSetState(RenderStates::RSWireframe.Get());
    }
    else
    {
        m_pd3dImmediateContext->RSSetState(nullptr);
    }

    // 应用常量缓冲区的变更
    m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
    // 除了索引为0的缓冲区缺少内部图元数目记录，其余都可以使用DrawAuto方法
    if (m_CurrIndex == 0)
    {
        m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
    }
    else
    {
        m_pd3dImmediateContext->DrawAuto();
    }
        
    // 绘制法向量
    if (m_ShowNormal)
    {
        m_BasicEffect.SetRenderNormal(m_pd3dImmediateContext.Get());
        m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
        // 除了索引为0的缓冲区缺少内部图元数目记录，其余都可以使用DrawAuto方法
        if (m_CurrIndex == 0)
        {
            m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
        }
        else
        {
            m_pd3dImmediateContext->DrawAuto();
        }
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, 0));

}



bool GameApp::InitResource()
{
    // ******************
    // 初始化对象
    //

    // 默认绘制三角形
    ResetSplitedTriangle();
    // 预先绑定顶点缓冲区
    UINT stride = sizeof(VertexPosColor);
    UINT offset = 0;
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[0].GetAddressOf(), &stride, &offset);

    // ******************
    // 初始化不会变化的值
    //

    // 方向光
    DirectionalLight dirLight{};
    dirLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    dirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight.direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
    m_BasicEffect.SetDirLight(0, dirLight);
    // 材质
    Material material{};
    material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    material.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
    m_BasicEffect.SetMaterial(material);
    // 摄像机位置
    m_BasicEffect.SetEyePos(XMFLOAT3(0.0f, 0.0f, -5.0f));
    // 矩阵
    m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
    m_BasicEffect.SetViewMatrix(XMMatrixLookAtLH(
        XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f),
        XMVectorZero(),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
    m_BasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));

    m_BasicEffect.SetSphereCenter(XMFLOAT3());
    m_BasicEffect.SetSphereRadius(2.0f);

    return true;
}


void GameApp::ResetSplitedTriangle()
{
    // ******************
    // 初始化三角形
    //

    // 设置三角形顶点
    VertexPosColor vertices[] =
    {
        { XMFLOAT3(-1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(0.0f * 3, 0.866f * 3, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
    };
    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_DEFAULT;	// 这里需要允许流输出阶段通过GPU写入
    vbd.ByteWidth = sizeof vertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// 需要额外添加流输出标签
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffers[0].ReleaseAndGetAddressOf()));

//#if defined(DEBUG) | defined(_DEBUG)
//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
//	graphicsAnalysis->BeginCapture();
//#endif

    // 三角形顶点数
    m_InitVertexCounts = 3;
    // 初始化所有顶点缓冲区
    for (int i = 1; i < 7; ++i)
    {
        vbd.ByteWidth *= 3;
        HR(m_pd3dDevice->CreateBuffer(&vbd, nullptr, m_pVertexBuffers[i].ReleaseAndGetAddressOf()));
        m_BasicEffect.SetStreamOutputSplitedTriangle(m_pd3dImmediateContext.Get(), m_pVertexBuffers[i - 1].Get(), m_pVertexBuffers[i].Get());
        // 第一次绘制需要调用一般绘制指令，之后就可以使用DrawAuto了
        if (i == 1)
        {
            m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
        }
        else
        {
            m_pd3dImmediateContext->DrawAuto();
        }

    }

//#if defined(DEBUG) | defined(_DEBUG)
//	graphicsAnalysis->EndCapture();
//#endif

    D3D11SetDebugObjectName(m_pVertexBuffers[0].Get(), "TriangleVertexBuffer[0]");
    D3D11SetDebugObjectName(m_pVertexBuffers[1].Get(), "TriangleVertexBuffer[1]");
    D3D11SetDebugObjectName(m_pVertexBuffers[2].Get(), "TriangleVertexBuffer[2]");
    D3D11SetDebugObjectName(m_pVertexBuffers[3].Get(), "TriangleVertexBuffer[3]");
    D3D11SetDebugObjectName(m_pVertexBuffers[4].Get(), "TriangleVertexBuffer[4]");
    D3D11SetDebugObjectName(m_pVertexBuffers[5].Get(), "TriangleVertexBuffer[5]");
    D3D11SetDebugObjectName(m_pVertexBuffers[6].Get(), "TriangleVertexBuffer[6]");
}

void GameApp::ResetSplitedSnow()
{
    // ******************
    // 雪花分形从初始化三角形开始，需要6个顶点
    //

    // 设置三角形顶点
    float sqrt3 = sqrtf(3.0f);
    VertexPosColor vertices[] =
    {
        { XMFLOAT3(-3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, sqrt3 / 2, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, sqrt3 / 2, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
    };
    // 将三角形宽度和高度都放大3倍
    for (VertexPosColor& v : vertices)
    {
        v.pos.x *= 3;
        v.pos.y *= 3;
    }

    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_DEFAULT;	// 这里需要允许流输出阶段通过GPU写入
    vbd.ByteWidth = sizeof vertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// 需要额外添加流输出标签
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffers[0].ReleaseAndGetAddressOf()));

//#if defined(DEBUG) | defined(_DEBUG)
//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
//	graphicsAnalysis->BeginCapture();
//#endif

    // 顶点数
    m_InitVertexCounts = 6;
    // 初始化所有顶点缓冲区
    for (int i = 1; i < 7; ++i)
    {
        vbd.ByteWidth *= 4;
        HR(m_pd3dDevice->CreateBuffer(&vbd, nullptr, m_pVertexBuffers[i].ReleaseAndGetAddressOf()));
        m_BasicEffect.SetStreamOutputSplitedSnow(m_pd3dImmediateContext.Get(), m_pVertexBuffers[i - 1].Get(), m_pVertexBuffers[i].Get());
        // 第一次绘制需要调用一般绘制指令，之后就可以使用DrawAuto了
        if (i == 1)
        {
            m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
        }
        else
        {
            m_pd3dImmediateContext->DrawAuto();
        }
    }

//#if defined(DEBUG) | defined(_DEBUG)
//	graphicsAnalysis->EndCapture();
//#endif

    D3D11SetDebugObjectName(m_pVertexBuffers[0].Get(), "SnowVertexBuffer[0]");
    D3D11SetDebugObjectName(m_pVertexBuffers[1].Get(), "SnowVertexBuffer[1]");
    D3D11SetDebugObjectName(m_pVertexBuffers[2].Get(), "SnowVertexBuffer[2]");
    D3D11SetDebugObjectName(m_pVertexBuffers[3].Get(), "SnowVertexBuffer[3]");
    D3D11SetDebugObjectName(m_pVertexBuffers[4].Get(), "SnowVertexBuffer[4]");
    D3D11SetDebugObjectName(m_pVertexBuffers[5].Get(), "SnowVertexBuffer[5]");
    D3D11SetDebugObjectName(m_pVertexBuffers[6].Get(), "SnowVertexBuffer[6]");
}

void GameApp::ResetSplitedSphere()
{
    // ******************
    // 分形球体
    //

    VertexPosNormalColor basePoint[] = {
        { XMFLOAT3(0.0f, 2.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(2.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-2.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, -2.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, -2.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
    };
    int indices[] = { 0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 1, 4, 1, 2, 5, 2, 3, 5, 3, 4, 5, 4, 1, 5 };

    std::vector<VertexPosNormalColor> vertices;
    for (int pos : indices)
    {
        vertices.push_back(basePoint[pos]);
    }


    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_DEFAULT;	// 这里需要允许流输出阶段通过GPU写入
    vbd.ByteWidth = (UINT)(vertices.size() * sizeof(VertexPosNormalColor));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// 需要额外添加流输出标签
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices.data();
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffers[0].ReleaseAndGetAddressOf()));

//#if defined(DEBUG) | defined(_DEBUG)
//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
//	graphicsAnalysis->BeginCapture();
//#endif

    // 顶点数
    m_InitVertexCounts = 24;
    // 初始化所有顶点缓冲区
    for (int i = 1; i < 7; ++i)
    {
        vbd.ByteWidth *= 4;
        HR(m_pd3dDevice->CreateBuffer(&vbd, nullptr, m_pVertexBuffers[i].ReleaseAndGetAddressOf()));
        m_BasicEffect.SetStreamOutputSplitedSphere(m_pd3dImmediateContext.Get(), m_pVertexBuffers[i - 1].Get(), m_pVertexBuffers[i].Get());
        // 第一次绘制需要调用一般绘制指令，之后就可以使用DrawAuto了
        if (i == 1)
        {
            m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
        }
        else
        {
            m_pd3dImmediateContext->DrawAuto();
        }
    }

//#if defined(DEBUG) | defined(_DEBUG)
//	graphicsAnalysis->EndCapture();
//#endif

    D3D11SetDebugObjectName(m_pVertexBuffers[0].Get(), "SphereVertexBuffer[0]");
    D3D11SetDebugObjectName(m_pVertexBuffers[1].Get(), "SphereVertexBuffer[1]");
    D3D11SetDebugObjectName(m_pVertexBuffers[2].Get(), "SphereVertexBuffer[2]");
    D3D11SetDebugObjectName(m_pVertexBuffers[3].Get(), "SphereVertexBuffer[3]");
    D3D11SetDebugObjectName(m_pVertexBuffers[4].Get(), "SphereVertexBuffer[4]");
    D3D11SetDebugObjectName(m_pVertexBuffers[5].Get(), "SphereVertexBuffer[5]");
    D3D11SetDebugObjectName(m_pVertexBuffers[6].Get(), "SphereVertexBuffer[6]");
}



