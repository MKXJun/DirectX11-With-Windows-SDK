#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
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

#ifndef USE_IMGUI
	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);
#endif

	return true;
}

void GameApp::OnResize()
{
	// 释放D2D的相关资源
	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

#ifndef USE_IMGUI
	// 为D2D创建DXGI表面渲染目标
	ComPtr<IDXGISurface> surface;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = m_pd2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, m_pd2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE)
	{
		OutputDebugStringW(L"\n警告：Direct2D与Direct3D互操作性功能受限，你将无法看到文本信息。现提供下述可选方法：\n"
			L"1. 对于Win7系统，需要更新至Win7 SP1，并安装KB2670838补丁以支持Direct2D显示。\n"
			L"2. 自行完成Direct3D 10.1与Direct2D的交互。详情参阅："
			L"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			L"3. 使用别的字体库，比如FreeType。\n\n");
	}
	else if (hr == S_OK)
	{
		// 创建固定颜色刷和文本格式
		HR(m_pd2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			m_pColorBrush.GetAddressOf()));
		HR(m_pdwriteFactory->CreateTextFormat(L"宋体", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
			m_pTextFormat.GetAddressOf()));
	}
	else
	{
		// 报告异常问题
		assert(m_pd2dRenderTarget);
	}
#endif

}

void GameApp::UpdateScene(float dt)
{
#ifdef USE_IMGUI
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
#else
	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
		m_TessellationMode = TessellationMode::Triangle;
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
		m_TessellationMode = TessellationMode::Quad;
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3))
		m_TessellationMode = TessellationMode::BezierCurve;
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D4))
		m_TessellationMode = TessellationMode::BezierSurface;
#endif

	switch (m_TessellationMode)
	{
	case GameApp::TessellationMode::Triangle: UpdateTriangle(); break;
	case GameApp::TessellationMode::Quad: UpdateQuad(); break;
	case GameApp::TessellationMode::BezierCurve: UpdateBezierCurve(); break;
	case GameApp::TessellationMode::BezierSurface: UpdateBezierSurface(); break;
	}

#ifdef USE_IMGUI
	ImGui::End();
	ImGui::Render();
#endif
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	switch (m_TessellationMode)
	{
	case GameApp::TessellationMode::Triangle: DrawTriangle(); break;
	case GameApp::TessellationMode::Quad: DrawQuad(); break;
	case GameApp::TessellationMode::BezierCurve: DrawBezierCurve(); break;
	case GameApp::TessellationMode::BezierSurface: DrawBezierSurface(); break;
	}

#ifdef USE_IMGUI
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#else
	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"1-三角形细分 2-四边形细分 3-贝塞尔曲线 4-贝塞尔曲面\n";
		
		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}
#endif

	HR(m_pSwapChain->Present(0, 0));
}

void GameApp::UpdateTriangle()
{
#ifdef USE_IMGUI
	ImGui::SliderFloat3("TriEdgeTess", m_TriEdgeTess, 1.0f, 10.0f, "%.1f");
	ImGui::SliderFloat("TriInsideTess", &m_TriInsideTess, 1.0f, 10.0f, "%.1f");
#else
	// *****************
	// 键盘操作
	//
	if (m_TriEdgeTess[0] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
		m_TriEdgeTess[0] -= 1.0f;
	if (m_TriEdgeTess[0] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::W))
		m_TriEdgeTess[0] += 1.0f;

	if (m_TriEdgeTess[1] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::A))
		m_TriEdgeTess[1] -= 1.0f;
	if (m_TriEdgeTess[1] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::S))
		m_TriEdgeTess[1] += 1.0f;

	if (m_TriEdgeTess[2] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::Z))
		m_TriEdgeTess[2] -= 1.0f;
	if (m_TriEdgeTess[2] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::X))
		m_TriEdgeTess[2] += 1.0f;

	if (m_TriInsideTess > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::E))
		m_TriInsideTess -= 1.0f;
	if (m_TriInsideTess < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::R))
		m_TriInsideTess += 1.0f;
#endif

	// *****************
	// 更新数据并应用
	//
	m_pEffectHelper->GetConstantBufferVariable("g_TriEdgeTess")->SetFloatVector(3, m_TriEdgeTess);
	m_pEffectHelper->GetConstantBufferVariable("g_TriInsideTess")->SetFloat(m_TriInsideTess);
}

void GameApp::UpdateQuad()
{
#ifdef USE_IMGUI
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
#else
	// *****************
	// 键盘操作
	//
	// 细分模式
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::T))
		m_PartitionMode = PartitionMode::Integer;
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Y))
		m_PartitionMode = PartitionMode::Odd;
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::U))
		m_PartitionMode = PartitionMode::Even;

	if (m_QuadEdgeTess[0] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::A))
		m_QuadEdgeTess[0] -= 0.25f;
	if (m_QuadEdgeTess[0] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::S))
		m_QuadEdgeTess[0] += 0.25f;

	if (m_QuadEdgeTess[1] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
		m_QuadEdgeTess[1] -= 0.25f;
	if (m_QuadEdgeTess[1] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::W))
		m_QuadEdgeTess[1] += 0.25f;

	if (m_QuadEdgeTess[2] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::E))
		m_QuadEdgeTess[2] -= 0.25f;
	if (m_QuadEdgeTess[2] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::R))
		m_QuadEdgeTess[2] += 0.25f;

	if (m_QuadEdgeTess[3] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::D))
		m_QuadEdgeTess[3] -= 0.25f;
	if (m_QuadEdgeTess[3] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::F))
		m_QuadEdgeTess[3] += 0.25f;

	if (m_QuadInsideTess[0] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::Z))
		m_QuadInsideTess[0] -= 0.25f;
	if (m_QuadInsideTess[0] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::X))
		m_QuadInsideTess[0] += 0.25f;

	if (m_QuadInsideTess[1] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::C))
		m_QuadInsideTess[1] -= 0.25f;
	if (m_QuadInsideTess[1] < 10.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::V))
		m_QuadInsideTess[1] += 0.25f;
#endif

	// *****************
	// 更新数据并应用
	//
	m_pEffectHelper->GetConstantBufferVariable("g_QuadEdgeTess")->SetFloatVector(4, m_QuadEdgeTess);
	m_pEffectHelper->GetConstantBufferVariable("g_QuadInsideTess")->SetFloatVector(2, m_QuadInsideTess);
}

void GameApp::UpdateBezierCurve()
{
	bool c1_continuity = false;
#ifdef USE_IMGUI
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
	else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_ChosenBezPoint >= 0)
		m_BezPoints[m_ChosenBezPoint] = worldPos;

#else
	// *****************
	// 键盘操作
	//

	// 控制细分程度
	if (m_IsolineEdgeTess[1] > 1.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
		m_IsolineEdgeTess[1] /= 2;
	if (m_IsolineEdgeTess[1] < 64.0f && m_KeyboardTracker.IsKeyPressed(Keyboard::W))
		m_IsolineEdgeTess[1] *= 2;
	// 端点连接处一阶连续
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::E))
		c1_continuity = true;

	// *****************
	// 检验是否有控制点被点击，并实现拖控
	//

	// 更新鼠标事件，获取位置
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);
	XMFLOAT3 worldPos = XMFLOAT3(
		(2.0f * mouseState.x / m_ClientWidth - 1.0f) * AspectRatio(),
		-2.0f * mouseState.y / m_ClientHeight + 1.0f,
		0.0f);
	float dy = 12.0f / m_ClientHeight;	// 稍微大一点方便点到

	switch (m_MouseTracker.leftButton)
	{
	case Mouse::ButtonStateTracker::UP: m_ChosenBezPoint = -1; break;
	case Mouse::ButtonStateTracker::PRESSED:
	{
		for (int i = 0; i < 10; ++i)
		{
			if (fabs(worldPos.x - m_BezPoints[i].x) <= dy && fabs(worldPos.y - m_BezPoints[i].y) <= dy)
			{
				m_ChosenBezPoint = i;
				break;
			}
		}
		break;
	}
	case Mouse::ButtonStateTracker::HELD: if (m_ChosenBezPoint >= 0) m_BezPoints[m_ChosenBezPoint] = worldPos;
		break;
	}
#endif

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
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		m_Theta = XMScalarModAngle(m_Theta + io.MouseDelta.x * 0.01f);
		m_Phi += -io.MouseDelta.y * 0.01f;
	}
	m_Radius -= io.MouseWheel;
#else
	// ******************
	// 更新摄像机位置
	//
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	if (mouseState.leftButton)
	{
		m_Theta = XMScalarModAngle(m_Theta + (mouseState.x - lastMouseState.x) * 0.01f);
		m_Phi += -(mouseState.y - lastMouseState.y) * 0.01f;
	}

	m_Radius += (lastMouseState.scrollWheelValue - mouseState.scrollWheelValue) / 120 * 1.0f;
#endif	

	// 限制Phi
	if (m_Phi < XM_PI / 18)
		m_Phi = XM_PI / 18;
	else if (m_Phi > XM_PI * 17 / 18)
		m_Phi = XM_PI * 17 / 18;

	// 限制半径
	if (m_Radius < 1.0f)
		m_Radius = 1.0f;
	else if (m_Radius > 100.0f)
		m_Radius = 100.0f;

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


#ifndef USE_IMGUI
	// ******************
	// 绘制Direct2D部分
	//
	m_pd2dRenderTarget->BeginDraw();
	{
		std::wstring wstr = L"(-Q/E+)EdgeTess[0] = " + std::to_wstring(m_TriEdgeTess[0]) + L"\n"
			L"(-A/D+)EdgeTess[1] = " + std::to_wstring(m_TriEdgeTess[1]) + L"\n"
			L"(-Z/X+)EdgeTess[2] = " + std::to_wstring(m_TriEdgeTess[2]) + L"\n"
			L"(-E/R+)InsideTess = " + std::to_wstring(m_TriInsideTess) + L"\n";

		m_pd2dRenderTarget->DrawTextW(wstr.c_str(), (UINT)wstr.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, m_ClientHeight * 0.2f, 300.0f, m_ClientHeight * 1.0f }, m_pColorBrush.Get());
	}
	HR(m_pd2dRenderTarget->EndDraw());
#endif
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

#ifndef USE_IMGUI
	// ******************
	// 绘制Direct2D部分
	//
	m_pd2dRenderTarget->BeginDraw();
	{
		std::wstring wstr =
			L"EdgeTess[0] = " + std::to_wstring(m_QuadEdgeTess[0]) + L"\n(-A/S+)\n"
			L"EdgeTess[1] = " + std::to_wstring(m_QuadEdgeTess[1]) + L"\n(-Q/W+)\n"
			L"EdgeTess[2] = " + std::to_wstring(m_QuadEdgeTess[2]) + L"\n(-E/R+)\n"
			L"EdgeTess[3] = " + std::to_wstring(m_QuadEdgeTess[3]) + L"\n(-D/F+)\n"
			L"InsideTess[0] = " + std::to_wstring(m_QuadInsideTess[0]) + L"\n(-Z/X+)\n"
			L"InsideTess[1] = " + std::to_wstring(m_QuadInsideTess[1]) + L"\n(-C/V+)\n";

		m_pd2dRenderTarget->DrawTextW(wstr.c_str(), (UINT)wstr.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, m_ClientHeight * 0.2f, 200.0f, m_ClientHeight * 1.0f }, m_pColorBrush.Get());

		wstr = L"T-Integer Y-Odd U-Even 当前划分: ";
		switch (m_PartitionMode)
		{
		case GameApp::PartitionMode::Integer: wstr += L"Integer"; break;
		case GameApp::PartitionMode::Odd: wstr += L"Odd"; break;
		case GameApp::PartitionMode::Even: wstr += L"Even"; break;
		}
		m_pd2dRenderTarget->DrawTextW(wstr.c_str(), (UINT)wstr.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 00.0f, 15.0f, 600.0f, 200.0f }, m_pColorBrush.Get());

	}
	HR(m_pd2dRenderTarget->EndDraw());
#endif
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

#ifndef USE_IMGUI
	// ******************
	// 绘制Direct2D部分
	//
	m_pd2dRenderTarget->BeginDraw();
	{
		std::wstring wstr = L"鼠标拖动控制点进行移动\n(-Q/W+)细分程度: " + std::to_wstring((int)m_IsolineEdgeTess[1]) + L"\n"
			L"E-端点处一阶连续";

		m_pd2dRenderTarget->DrawTextW(wstr.c_str(), (UINT)wstr.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 15.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
	}
	HR(m_pd2dRenderTarget->EndDraw());
#endif
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

#ifndef USE_IMGUI
	m_pd2dRenderTarget->BeginDraw();
	{
		std::wstring wstr = L"鼠标拖动控制视野 滚轮控制距离";
		m_pd2dRenderTarget->DrawTextW(wstr.c_str(), (UINT)wstr.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 15.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
	}
	HR(m_pd2dRenderTarget->EndDraw());
#endif
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
	HR(CreateVertexBuffer(m_pd3dDevice.Get(), triVertices, sizeof triVertices, m_pTriangleVB.GetAddressOf()));

	XMFLOAT3 quadVertices[4] = {
		XMFLOAT3(-0.4f, 0.72f, 0.0f),
		XMFLOAT3(0.4f, 0.72f, 0.0f),
		XMFLOAT3(-0.4f, -0.72f, 0.0f),
		XMFLOAT3(0.4f, -0.72f, 0.0f)
	};
	HR(CreateVertexBuffer(m_pd3dDevice.Get(), quadVertices, sizeof quadVertices, m_pQuadVB.GetAddressOf()));

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
	HR(CreateVertexBuffer(m_pd3dDevice.Get(), nullptr, sizeof(XMFLOAT3) * 12, m_pBezCurveVB.GetAddressOf(), true));

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
	HR(CreateVertexBuffer(m_pd3dDevice.Get(), surfaceVertices, sizeof surfaceVertices, m_pBezSurfaceVB.GetAddressOf()));

	// ******************
	// 创建光栅化状态
	//
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));

	// 线框模式
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthClipEnable = true;
	HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRSWireFrame.GetAddressOf()));
	
	// ******************
	// 创建着色器和顶点输入布局
	//

	ComPtr<ID3DBlob> blob;
	D3D11_INPUT_ELEMENT_DESC inputElemDesc[1] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	HR(CreateShaderFromFile(L"HLSL\\Tessellation_VS.cso", L"HLSL\\Tessellation_VS.hlsl", "VS", "vs_5_0", blob.GetAddressOf()));
	HR(m_pd3dDevice->CreateInputLayout(inputElemDesc, 1, blob->GetBufferPointer(), blob->GetBufferSize(), m_pInputLayout.GetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_VS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Transform_VS.cso", L"HLSL\\Tessellation_Transform_VS.hlsl", "VS", "vs_5_0", blob.GetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Transform_VS", m_pd3dDevice.Get(), blob.Get()));

	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Isoline_HS.cso", L"HLSL\\Tessellation_Isoline_HS.hlsl", "HS", "hs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Isoline_HS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Triangle_HS.cso", L"HLSL\\Tessellation_Triangle_HS.hlsl", "HS", "hs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Triangle_HS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Quad_Integer_HS.cso", L"HLSL\\Tessellation_Quad_Integer_HS.hlsl", "HS", "hs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Quad_Integer_HS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Quad_Odd_HS.cso", L"HLSL\\Tessellation_Quad_Odd_HS.hlsl", "HS", "hs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Quad_Odd_HS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Quad_Even_HS.cso", L"HLSL\\Tessellation_Quad_Even_HS.hlsl", "HS", "hs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Quad_Even_HS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_BezierSurface_HS.cso", L"HLSL\\Tessellation_BezierSurface_HS.hlsl", "HS", "ds_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_BezierSurface_HS", m_pd3dDevice.Get(), blob.Get()));

	HR(CreateShaderFromFile(L"HLSL\\Tessellation_BezierCurve_DS.cso", L"HLSL\\Tessellation_BezierCurve_DS.hlsl", "DS", "ds_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_BezierCurve_DS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Triangle_DS.cso", L"HLSL\\Tessellation_Triangle_DS.hlsl", "DS", "ds_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Triangle_DS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Quad_DS.cso", L"HLSL\\Tessellation_Quad_DS.hlsl", "DS", "ds_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Quad_DS", m_pd3dDevice.Get(), blob.Get()));
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_BezierSurface_DS.cso", L"HLSL\\Tessellation_BezierSurface_DS.hlsl", "DS", "ds_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_BezierSurface_DS", m_pd3dDevice.Get(), blob.Get()));
	
	HR(CreateShaderFromFile(L"HLSL\\Tessellation_Point2Square_GS.cso", L"HLSL\\Tessellation_Point2Square_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_Point2Square_GS", m_pd3dDevice.Get(), blob.Get()));

	HR(CreateShaderFromFile(L"HLSL\\Tessellation_PS.cso", L"HLSL\\Tessellation_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pEffectHelper->AddShader("Tessellation_PS", m_pd3dDevice.Get(), blob.Get()));

	EffectPassDesc passDesc;
	passDesc.nameVS = "Tessellation_Transform_VS";
	passDesc.namePS = "Tessellation_PS";
	HR(m_pEffectHelper->AddEffectPass("Tessellation_NoTess", m_pd3dDevice.Get(), &passDesc));

	passDesc.nameVS = "Tessellation_VS";
	passDesc.nameGS = "Tessellation_Point2Square_GS";
	passDesc.namePS = "Tessellation_PS";
	HR(m_pEffectHelper->AddEffectPass("Tessellation_Point2Square", m_pd3dDevice.Get(), &passDesc));
	passDesc.nameGS = nullptr;

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

