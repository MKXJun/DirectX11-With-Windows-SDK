#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>
#include <DirectXColors.h>
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight),
    m_pEffectHelper(std::make_unique<EffectHelper>())
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{
    D3DApp::OnResize();
}

void GameApp::UpdateScene(float dt)
{

    if (ImGui::Begin("Tessellation"))
    {

        static int curr_item = 0;
        static const char* modes[] = {
            "Triangle",
            "Quad",
            "BezierCurve",
            "BezierSurface"
        };
        if (ImGui::Combo("Mode", &curr_item, modes, ARRAYSIZE(modes)))
        {
            m_TessellationMode = static_cast<TessellationMode>(curr_item);
        }
    }

    switch (m_TessellationMode)
    {
    case GameApp::TessellationMode::Triangle: UpdateTriangle(); break;
    case GameApp::TessellationMode::Quad: UpdateQuad(); break;
    case GameApp::TessellationMode::BezierCurve: UpdateBezierCurve(); break;
    case GameApp::TessellationMode::BezierSurface: UpdateBezierSurface(); break;
    }

    ImGui::End();
    ImGui::Render();
}

void GameApp::DrawScene()
{
    if (m_FrameCount < m_BackBufferCount)
    {
        ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, m_pRenderTargetViews[m_FrameCount].ReleaseAndGetAddressOf());
    }
    
    m_pd3dImmediateContext->ClearRenderTargetView(GetBackBufferRTV(), Colors::Black);
    ID3D11RenderTargetView* pRTVs[1] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
    CD3D11_VIEWPORT viewport(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    switch (m_TessellationMode)
    {
    case GameApp::TessellationMode::Triangle: DrawTriangle(); break;
    case GameApp::TessellationMode::Quad: DrawQuad(); break;
    case GameApp::TessellationMode::BezierCurve: DrawBezierCurve(); break;
    case GameApp::TessellationMode::BezierSurface: DrawBezierSurface(); break;
    }


    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}

void GameApp::UpdateTriangle()
{
    ImGui::SliderFloat3("TriEdgeTess", m_TriEdgeTess, 1.0f, 10.0f, "%.1f");
    ImGui::SliderFloat("TriInsideTess", &m_TriInsideTess, 1.0f, 10.0f, "%.1f");

    // *****************
    // 更新数据并应用
    //
    m_pEffectHelper->GetConstantBufferVariable("g_TriEdgeTess")->SetFloatVector(3, m_TriEdgeTess);
    m_pEffectHelper->GetConstantBufferVariable("g_TriInsideTess")->SetFloat(m_TriInsideTess);
}

void GameApp::UpdateQuad()
{

    static int part_item = 0;
    static const char* part_modes[] = {
        "Integer",
        "Odd",
        "Even"
    };
    if (ImGui::Combo("Partition Mode", &part_item, part_modes, ARRAYSIZE(part_modes)))
        m_PartitionMode = static_cast<PartitionMode>(part_item);
    ImGui::SliderFloat4("QuadEdgeTess", m_QuadEdgeTess, 1.0f, 10.0f, "%.1f");
    ImGui::SliderFloat2("QuadInsideTess", m_QuadInsideTess, 1.0f, 10.0f, "%.1f");
    m_pEffectHelper->GetConstantBufferVariable("g_QuadEdgeTess")->SetFloatVector(4, m_QuadEdgeTess);
    m_pEffectHelper->GetConstantBufferVariable("g_QuadInsideTess")->SetFloatVector(2, m_QuadInsideTess);

    // *****************
    // 更新数据并应用
    //
    m_pEffectHelper->GetConstantBufferVariable("g_QuadEdgeTess")->SetFloatVector(4, m_QuadEdgeTess);
    m_pEffectHelper->GetConstantBufferVariable("g_QuadInsideTess")->SetFloatVector(2, m_QuadInsideTess);
}

void GameApp::UpdateBezierCurve()
{
    bool c1_continuity = false;

    ImGui::SliderFloat("IsolineEdgeTess", &m_IsolineEdgeTess[1], 1.0f, 64.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
    if (ImGui::Button("C1 Continuity"))
        c1_continuity = true;
    ImGuiIO& io = ImGui::GetIO();

    XMFLOAT3 worldPos = XMFLOAT3(
        (2.0f * io.MousePos.x / m_ClientWidth - 1.0f) * AspectRatio(),
        -2.0f * io.MousePos.y / m_ClientHeight + 1.0f,
        0.0f);
    float dy = 12.0f / m_ClientHeight;	// 稍微大一点方便点到
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        for (int i = 0; i < 10; ++i)
        {
            if (fabs(worldPos.x - m_BezPoints[i].x) <= dy && fabs(worldPos.y - m_BezPoints[i].y) <= dy)
            {
                m_ChosenBezPoint = i;
                break;
            }
        }
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        m_ChosenBezPoint = -1;
    else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_ChosenBezPoint >= 0 && 
        io.MousePos.x > 5 && io.MousePos.x < m_ClientWidth - 5 &&
        io.MousePos.y > 5 && io.MousePos.y < m_ClientHeight - 5)
        m_BezPoints[m_ChosenBezPoint] = worldPos;

    // *****************
    // 更新数据并应用
    //
    if (c1_continuity)
    {
        XMVECTOR posVec2 = XMLoadFloat3(&m_BezPoints[2]);
        XMVECTOR posVec3 = XMLoadFloat3(&m_BezPoints[3]);
        XMVECTOR posVec5 = XMLoadFloat3(&m_BezPoints[5]);
        XMVECTOR posVec6 = XMLoadFloat3(&m_BezPoints[6]);
        XMStoreFloat3(m_BezPoints + 4, posVec2 + 2 * (posVec3 - posVec2));
        XMStoreFloat3(m_BezPoints + 7, posVec5 + 2 * (posVec6 - posVec5));
    }

    XMMATRIX WVP = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), g_XMZero, g_XMIdentityR1) *
        XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 0.1f, 1000.0f);
    WVP = XMMatrixTranspose(WVP);
    m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);
    m_pEffectHelper->GetConstantBufferVariable("g_IsolineEdgeTess")->SetFloatVector(2, m_IsolineEdgeTess);
    m_pEffectHelper->GetConstantBufferVariable("g_InvScreenHeight")->SetFloat(1.0f / m_ClientHeight);

    D3D11_MAPPED_SUBRESOURCE mappedData;
    HR(m_pd3dImmediateContext->Map(m_pBezCurveVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    for (size_t i = 0; i < 3; ++i)
    {
        memcpy_s(reinterpret_cast<XMFLOAT3*>(mappedData.pData) + i * 4, sizeof(XMFLOAT3) * 4, m_BezPoints + i * 3, sizeof(XMFLOAT3) * 4);
    }
    m_pd3dImmediateContext->Unmap(m_pBezCurveVB.Get(), 0);

}

void GameApp::UpdateBezierSurface()
{
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        m_Theta = XMScalarModAngle(m_Theta + io.MouseDelta.x * 0.01f);
        m_Phi += -io.MouseDelta.y * 0.01f;
    }
    m_Radius -= io.MouseWheel;

    // 限制Phi
    m_Phi = std::clamp(m_Phi, XM_PI / 18, 1.0f - XM_PI / 18);
    // 限制半径
    m_Radius = std::clamp(m_Radius, 1.0f, 100.0f);

    XMVECTOR posVec = XMVectorSet(
        m_Radius * sinf(m_Phi) * cosf(m_Theta),
        m_Radius * cosf(m_Phi),
        m_Radius * sinf(m_Phi) * sinf(m_Theta),
        0.0f);

    // *****************
    // 更新数据并应用
    //
    XMMATRIX WVP = XMMatrixLookAtLH(posVec, g_XMZero, g_XMIdentityR1) *
        XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 0.1f, 1000.0f);
    WVP = XMMatrixTranspose(WVP);
    m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);
    m_pEffectHelper->GetConstantBufferVariable("g_QuadEdgeTess")->SetFloatVector(4, m_QuadPatchEdgeTess);
    m_pEffectHelper->GetConstantBufferVariable("g_QuadInsideTess")->SetFloatVector(2, m_QuadPatchInsideTess);
}

void GameApp::DrawTriangle()
{
    // ******************
    // 绘制Direct3D部分
    //
    UINT strides[1] = { sizeof(XMFLOAT3) };
    UINT offsets[1] = { 0 };
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pTriangleVB.GetAddressOf(), strides, offsets);
    m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout.Get());
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

    m_pEffectHelper->GetConstantBufferVariable("g_Color")->SetFloatVector(4, Colors::White);
    m_pEffectHelper->GetEffectPass("Tessellation_Triangle")->Apply(m_pd3dImmediateContext.Get());

    m_pd3dImmediateContext->Draw(3, 0);

}

void GameApp::DrawQuad()
{
    // ******************
    // 绘制Direct3D部分
    //
    UINT strides[1] = { sizeof(XMFLOAT3) };
    UINT offsets[1] = { 0 };
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), strides, offsets);
    m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout.Get());
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

    m_pEffectHelper->GetConstantBufferVariable("g_Color")->SetFloatVector(4, Colors::White);
    switch (m_PartitionMode)
    {
    case PartitionMode::Integer: m_pEffectHelper->GetEffectPass("Tessellation_Quad_Integer")->Apply(m_pd3dImmediateContext.Get()); break;
    case PartitionMode::Odd: m_pEffectHelper->GetEffectPass("Tessellation_Quad_Odd")->Apply(m_pd3dImmediateContext.Get()); break;
    case PartitionMode::Even: m_pEffectHelper->GetEffectPass("Tessellation_Quad_Even")->Apply(m_pd3dImmediateContext.Get()); break;
    }

    m_pd3dImmediateContext->Draw(4, 0);
}

void GameApp::DrawBezierCurve()
{
    // ******************
    // 绘制Direct3D部分
    //
    UINT strides[1] = { sizeof(XMFLOAT3) };
    UINT offsets[1] = { 0 };
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pBezCurveVB.GetAddressOf(), strides, offsets);
    m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout.Get());

    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    m_pEffectHelper->GetConstantBufferVariable("g_Color")->SetFloatVector(4, Colors::Red);
    m_pEffectHelper->GetEffectPass("Tessellation_Point2Square")->Apply(m_pd3dImmediateContext.Get());
    m_pd3dImmediateContext->Draw(12, 0);

    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
    m_pEffectHelper->GetConstantBufferVariable("g_Color")->SetFloatVector(4, Colors::White);
    m_pEffectHelper->GetEffectPass("Tessellation_BezierCurve")->Apply(m_pd3dImmediateContext.Get());
    m_pd3dImmediateContext->Draw(12, 0);

    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    m_pEffectHelper->GetConstantBufferVariable("g_Color")->SetFloatVector(4, Colors::Red);
    m_pEffectHelper->GetEffectPass("Tessellation_NoTess")->Apply(m_pd3dImmediateContext.Get());
    m_pd3dImmediateContext->Draw(12, 0);
}

void GameApp::DrawBezierSurface()
{
    UINT strides[1] = { sizeof(XMFLOAT3) };
    UINT offsets[1] = { 0 };
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pBezSurfaceVB.GetAddressOf(), strides, offsets);
    m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout.Get());
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);

    m_pEffectHelper->GetConstantBufferVariable("g_Color")->SetFloatVector(4, Colors::White);
    m_pEffectHelper->GetEffectPass("Tessellation_BezierSurface")->Apply(m_pd3dImmediateContext.Get());

    m_pd3dImmediateContext->Draw(16, 0);
}

bool GameApp::InitResource()
{
    // ******************
    // 创建缓冲区
    //
    XMFLOAT3 triVertices[3] = {
        XMFLOAT3(-0.6f, -0.8f, 0.0f),
        XMFLOAT3(0.0f, 0.8f, 0.0f),
        XMFLOAT3(0.6f, -0.8f, 0.0f)
    };
    CD3D11_BUFFER_DESC bufferDesc(sizeof triVertices, D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA initData{ triVertices };
    HR(m_pd3dDevice->CreateBuffer(&bufferDesc, &initData, m_pTriangleVB.GetAddressOf()));

    XMFLOAT3 quadVertices[4] = {
        XMFLOAT3(-0.4f, 0.72f, 0.0f),
        XMFLOAT3(0.4f, 0.72f, 0.0f),
        XMFLOAT3(-0.4f, -0.72f, 0.0f),
        XMFLOAT3(0.4f, -0.72f, 0.0f)
    };
    bufferDesc.ByteWidth = sizeof quadVertices;
    initData.pSysMem = quadVertices;
    HR(m_pd3dDevice->CreateBuffer(&bufferDesc, &initData, m_pQuadVB.GetAddressOf()));

    XMFLOAT3 surfaceVertices[16] = {
        // 行 0
        XMFLOAT3(-10.0f, -10.0f, +15.0f),
        XMFLOAT3(-5.0f,  0.0f, +15.0f),
        XMFLOAT3(+5.0f,  0.0f, +15.0f),
        XMFLOAT3(+10.0f, 0.0f, +15.0f),

        // 行 1
        XMFLOAT3(-15.0f, 0.0f, +5.0f),
        XMFLOAT3(-5.0f,  0.0f, +5.0f),
        XMFLOAT3(+5.0f,  20.0f, +5.0f),
        XMFLOAT3(+15.0f, 0.0f, +5.0f),

        // 行 2
        XMFLOAT3(-15.0f, 0.0f, -5.0f),
        XMFLOAT3(-5.0f,  0.0f, -5.0f),
        XMFLOAT3(+5.0f,  0.0f, -5.0f),
        XMFLOAT3(+15.0f, 0.0f, -5.0f),

        // 行 3
        XMFLOAT3(-10.0f, 10.0f, -15.0f),
        XMFLOAT3(-5.0f,  0.0f, -15.0f),
        XMFLOAT3(+5.0f,  0.0f, -15.0f),
        XMFLOAT3(+25.0f, 10.0f, -15.0f)
    };
    bufferDesc.ByteWidth = sizeof surfaceVertices;
    initData.pSysMem = surfaceVertices;
    HR(m_pd3dDevice->CreateBuffer(&bufferDesc, &initData, m_pBezSurfaceVB.GetAddressOf()));

    m_BezPoints[0] = XMFLOAT3(-0.8f, -0.2f, 0.0f);
    m_BezPoints[1] = XMFLOAT3(-0.5f, -0.5f, 0.0f);
    m_BezPoints[2] = XMFLOAT3(-0.5f, 0.6f, 0.0f);
    m_BezPoints[3] = XMFLOAT3(-0.35f, 0.6f, 0.0f);
    m_BezPoints[4] = XMFLOAT3(-0.2f, 0.6f, 0.0f);
    m_BezPoints[5] = XMFLOAT3(0.1f, 0.0f, 0.0f);
    m_BezPoints[6] = XMFLOAT3(0.1f, -0.3f, 0.0f);
    m_BezPoints[7] = XMFLOAT3(0.4f, -0.3f, 0.0f);
    m_BezPoints[8] = XMFLOAT3(0.6f, 0.6f, 0.0f);
    m_BezPoints[9] = XMFLOAT3(0.8f, 0.4f, 0.0f);
    
    // 动态更新
    bufferDesc.ByteWidth = sizeof(XMFLOAT3) * 12;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HR(m_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, m_pBezCurveVB.GetAddressOf()));


    // ******************
    // 创建光栅化状态
    //
    CD3D11_RASTERIZER_DESC rasterizerDesc(CD3D11_DEFAULT{});
    // 线框模式
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRSWireFrame.GetAddressOf()));
    
    // ******************
    // 创建着色器和顶点输入布局
    //

    ComPtr<ID3DBlob> blob;
    D3D11_INPUT_ELEMENT_DESC inputElemDesc[1] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    m_pEffectHelper->SetBinaryCacheDirectory(L"Shaders\\Cache");


    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_VS", L"Shaders\\Tessellation_VS.hlsl", 
        m_pd3dDevice.Get(), "VS", "vs_5_0", nullptr, blob.GetAddressOf()));
    HR(m_pd3dDevice->CreateInputLayout(inputElemDesc, 1, blob->GetBufferPointer(), blob->GetBufferSize(), m_pInputLayout.GetAddressOf()));

    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Transform_VS", L"Shaders\\Tessellation_Transform_VS.hlsl",
        m_pd3dDevice.Get(), "VS", "vs_5_0"));

    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Isoline_HS", L"Shaders\\Tessellation_Isoline_HS.hlsl",
        m_pd3dDevice.Get(), "HS", "hs_5_0"));
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Triangle_HS", L"Shaders\\Tessellation_Triangle_HS.hlsl",
        m_pd3dDevice.Get(), "HS", "hs_5_0"));
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Quad_Integer_HS", L"Shaders\\Tessellation_Quad_Integer_HS.hlsl",
        m_pd3dDevice.Get(), "HS", "hs_5_0"));
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Quad_Odd_HS", L"Shaders\\Tessellation_Quad_Odd_HS.hlsl",
        m_pd3dDevice.Get(), "HS", "hs_5_0"));
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Quad_Even_HS", L"Shaders\\Tessellation_Quad_Even_HS.hlsl",
        m_pd3dDevice.Get(), "HS", "hs_5_0"));
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_BezierSurface_HS", L"Shaders\\Tessellation_BezierSurface_HS.hlsl",
        m_pd3dDevice.Get(), "HS", "hs_5_0"));

    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_BezierCurve_DS", L"Shaders\\Tessellation_BezierCurve_DS.hlsl",
        m_pd3dDevice.Get(), "DS", "ds_5_0"));
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Triangle_DS", L"Shaders\\Tessellation_Triangle_DS.hlsl",
        m_pd3dDevice.Get(), "DS", "ds_5_0"));
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Quad_DS", L"Shaders\\Tessellation_Quad_DS.hlsl",
        m_pd3dDevice.Get(), "DS", "ds_5_0"));
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_BezierSurface_DS", L"Shaders\\Tessellation_BezierSurface_DS.hlsl",
        m_pd3dDevice.Get(), "DS", "ds_5_0"));

    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_Point2Square_GS", L"Shaders\\Tessellation_Point2Square_GS.hlsl",
        m_pd3dDevice.Get(), "GS", "gs_5_0"));
    
    HR(m_pEffectHelper->CreateShaderFromFile("Tessellation_PS", L"Shaders\\Tessellation_PS.hlsl",
        m_pd3dDevice.Get(), "PS", "ps_5_0"));

    EffectPassDesc passDesc;
    passDesc.nameVS = "Tessellation_Transform_VS";
    passDesc.namePS = "Tessellation_PS";
    HR(m_pEffectHelper->AddEffectPass("Tessellation_NoTess", m_pd3dDevice.Get(), &passDesc));

    passDesc.nameVS = "Tessellation_VS";
    passDesc.nameGS = "Tessellation_Point2Square_GS";
    passDesc.namePS = "Tessellation_PS";
    HR(m_pEffectHelper->AddEffectPass("Tessellation_Point2Square", m_pd3dDevice.Get(), &passDesc));
    passDesc.nameGS = "";

    passDesc.nameVS = "Tessellation_VS";
    passDesc.nameHS = "Tessellation_Isoline_HS";
    passDesc.nameDS = "Tessellation_BezierCurve_DS";
    passDesc.namePS = "Tessellation_PS";
    HR(m_pEffectHelper->AddEffectPass("Tessellation_BezierCurve", m_pd3dDevice.Get(), &passDesc));
    m_pEffectHelper->GetEffectPass("Tessellation_BezierCurve")->SetRasterizerState(m_pRSWireFrame.Get());

    passDesc.nameVS = "Tessellation_VS";
    passDesc.nameHS = "Tessellation_Triangle_HS";
    passDesc.nameDS = "Tessellation_Triangle_DS";
    passDesc.namePS = "Tessellation_PS";
    HR(m_pEffectHelper->AddEffectPass("Tessellation_Triangle", m_pd3dDevice.Get(), &passDesc));
    m_pEffectHelper->GetEffectPass("Tessellation_Triangle")->SetRasterizerState(m_pRSWireFrame.Get());

    passDesc.nameVS = "Tessellation_VS";
    passDesc.nameHS = "Tessellation_Quad_Integer_HS";
    passDesc.nameDS = "Tessellation_Quad_DS";
    passDesc.namePS = "Tessellation_PS";
    HR(m_pEffectHelper->AddEffectPass("Tessellation_Quad_Integer", m_pd3dDevice.Get(), &passDesc));
    m_pEffectHelper->GetEffectPass("Tessellation_Quad_Integer")->SetRasterizerState(m_pRSWireFrame.Get());

    passDesc.nameVS = "Tessellation_VS";
    passDesc.nameHS = "Tessellation_Quad_Odd_HS";
    passDesc.nameDS = "Tessellation_Quad_DS";
    passDesc.namePS = "Tessellation_PS";
    HR(m_pEffectHelper->AddEffectPass("Tessellation_Quad_Odd", m_pd3dDevice.Get(), &passDesc));
    m_pEffectHelper->GetEffectPass("Tessellation_Quad_Odd")->SetRasterizerState(m_pRSWireFrame.Get());

    passDesc.nameVS = "Tessellation_VS";
    passDesc.nameHS = "Tessellation_Quad_Even_HS";
    passDesc.nameDS = "Tessellation_Quad_DS";
    passDesc.namePS = "Tessellation_PS";
    HR(m_pEffectHelper->AddEffectPass("Tessellation_Quad_Even", m_pd3dDevice.Get(), &passDesc));
    m_pEffectHelper->GetEffectPass("Tessellation_Quad_Even")->SetRasterizerState(m_pRSWireFrame.Get());

    passDesc.nameVS = "Tessellation_VS";
    passDesc.nameHS = "Tessellation_BezierSurface_HS";
    passDesc.nameDS = "Tessellation_BezierSurface_DS";
    passDesc.namePS = "Tessellation_PS";
    HR(m_pEffectHelper->AddEffectPass("Tessellation_BezierSurface", m_pd3dDevice.Get(), &passDesc));
    m_pEffectHelper->GetEffectPass("Tessellation_BezierSurface")->SetRasterizerState(m_pRSWireFrame.Get());

    m_pEffectHelper->SetDebugObjectName("Tessellation");

    return true;
}

