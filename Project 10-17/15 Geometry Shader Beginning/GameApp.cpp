#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight),
    m_ShowMode(Mode::SplitedTriangle),
    m_VertexCount()
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
    // 更新每帧变化的值
    if (m_ShowMode == Mode::SplitedTriangle)
    {
        m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
    }
    else
    {
        static float phi = 0.0f, theta = 0.0f;
        phi += 0.2f * dt, theta += 0.3f * dt;
        m_BasicEffect.SetWorldMatrix(XMMatrixRotationX(phi) * XMMatrixRotationY(theta));
    }

    if (ImGui::Begin("Geometry Shader Beginning"))
    {
        static int curr_item = 0;
        static const char* modes[] = {
            "Splited Triangle",
            "Cylinder w/o Cap"
        };
        if (ImGui::Combo("Mode", &curr_item, modes, ARRAYSIZE(modes)))
        {
            m_ShowMode = static_cast<Mode>(curr_item);
            if (curr_item == 0)
            {
                ResetTriangle();
                m_BasicEffect.SetRenderSplitedTriangle(m_pd3dImmediateContext.Get());
            }
            else
            {
                ResetRoundWire();
                m_BasicEffect.SetRenderCylinderNoCap(m_pd3dImmediateContext.Get());
            }

            // 输入装配阶段的顶点缓冲区设置
            UINT stride = curr_item ? sizeof(VertexPosNormalColor) : sizeof(VertexPosColor);		// 跨越字节数
            UINT offset = 0;																		// 起始偏移量
            m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

        }

        if (curr_item == 1)
        {
            static bool show_normal = false;
            if (ImGui::Checkbox("Show Normal", &show_normal))
            {
                m_ShowMode = show_normal ? Mode::CylinderNoCapWithNormal : Mode::CylinderNoCap;
            }
        }
    }
    ImGui::End();
    ImGui::Render();

}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // 应用常量缓冲区的变化
    m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
    m_pd3dImmediateContext->Draw(m_VertexCount, 0);
    // 绘制法向量，绘制完后记得归位
    if (m_ShowMode == Mode::CylinderNoCapWithNormal)
    {
        m_BasicEffect.SetRenderNormal(m_pd3dImmediateContext.Get());
        // 应用常量缓冲区的变化
        m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
        m_pd3dImmediateContext->Draw(m_VertexCount, 0);
        m_BasicEffect.SetRenderCylinderNoCap(m_pd3dImmediateContext.Get());
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
    ResetTriangle();
    
    // ******************
    // 初始化不会变化的值
    //

    // 方向光
    DirectionalLight dirLight;
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
    // 圆柱高度
    m_BasicEffect.SetCylinderHeight(2.0f);




    // 输入装配阶段的顶点缓冲区设置
    UINT stride = sizeof(VertexPosColor);		// 跨越字节数
    UINT offset = 0;							// 起始偏移量
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
    // 设置默认渲染状态
    m_BasicEffect.SetRenderSplitedTriangle(m_pd3dImmediateContext.Get());


    return true;
}


void GameApp::ResetTriangle()
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
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof vertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf()));
    // 三角形顶点数
    m_VertexCount = 3;

    // 设置调试对象名
    D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "TriangleVertexBuffer");
}

void GameApp::ResetRoundWire()
{
    // ****************** 
    // 初始化圆线
    // 设置圆边上各顶点
    // 必须要按顺时针设置
    // 由于要形成闭环，起始点需要使用2次
    //  ______
    // /      \ 
    // \______/
    //

    VertexPosNormalColor vertices[41];
    for (int i = 0; i < 40; ++i)
    {
        vertices[i].pos = XMFLOAT3(cosf(XM_PI / 20 * i), -1.0f, -sinf(XM_PI / 20 * i));
        vertices[i].normal = XMFLOAT3(cosf(XM_PI / 20 * i), 0.0f, -sinf(XM_PI / 20 * i));
        vertices[i].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    vertices[40] = vertices[0];

    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof vertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf()));
    // 线框顶点数
    m_VertexCount = 41;

    // 设置调试对象名
    D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "CylinderVertexBuffer");
}



