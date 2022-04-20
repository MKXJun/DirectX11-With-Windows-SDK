#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

#pragma warning(disable: 26812)

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance), 
	m_pBasicEffect(std::make_unique<BasicEffect>()),
	m_pSkyEffect(std::make_unique<SkyEffect>()),
	m_pShadowEffect(std::make_unique<ShadowEffect>()),
	m_pDebugEffect(std::make_unique<DebugEffect>()),
	m_pSSAOEffect(std::make_unique<SSAOEffect>()),
	m_RenderMode(RenderMode::DisplacementMap),
	m_FillMode(IEffect::Solid),
	m_EnableSSAO(true),
	m_EnableDebug(true),
	m_GrayMode(true),
	m_HeightScale(7)
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

	if (!m_pBasicEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_pSkyEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_pShadowEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_pSSAOEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_pDebugEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

#ifndef USE_IMGUI
	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);
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

	// 摄像机变更显示
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_pBasicEffect->SetProjMatrix(m_pCamera->GetProjXM());
		m_pSSAOEffect->SetProjMatrix(m_pCamera->GetProjXM());
	}
	
	// SSAO图和调试用矩形重新设置
	if (m_pSSAOMap)
	{
		m_pSSAOMap->OnResize(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, XM_PI / 3, 1000.0f);

		Model quadModel;
		quadModel.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow<VertexPosNormalTex>(XMFLOAT2(0.666666f, 0.666666f), XMFLOAT2(0.333333f, 0.333333f)));
		quadModel.modelParts[0].texDiffuse = m_pSSAOMap->GetAmbientTexture();
		m_DebugQuad.SetModel(std::move(quadModel));
	}

}

void GameApp::UpdateScene(float dt)
{
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	// ******************
	// 自由摄像机的操作
	//
	float d1 = 0.0f, d2 = 0.0f;
	if (ImGui::IsKeyDown('W'))
		d1 += dt;
	if (ImGui::IsKeyDown('S'))
		d1 -= dt;
	if (ImGui::IsKeyDown('A'))
		d2 -= dt;
	if (ImGui::IsKeyDown('D'))
		d2 += dt;

	cam1st->MoveForward(d1 * 6.0f);
	cam1st->Strafe(d2 * 6.0f);

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		cam1st->Pitch(io.MouseDelta.y * 0.01f);
		cam1st->RotateY(io.MouseDelta.x * 0.01f);
	}

	if (ImGui::Begin("Displacement Mapping"))
	{
		static int curr_item = 2;
		static const char* modes[] = {
			"Basic",
			"Normal Map",
			"Displacement Map"
		};
		if (ImGui::Combo("Mode", &curr_item, modes, ARRAYSIZE(modes)))
			m_RenderMode = static_cast<RenderMode>(curr_item);
		if (m_RenderMode == RenderMode::DisplacementMap)
			ImGui::SliderInt("Height Scale", &m_HeightScale, 0, 15);
		static bool isWireframe = false;
		if (ImGui::Checkbox("Wireframe Mode", &isWireframe))
			m_FillMode = static_cast<IEffect::RSFillMode>(!!isWireframe);
		if (!isWireframe)
		{
			if (ImGui::Checkbox("Enable SSAO", &m_EnableSSAO) && m_EnableDebug)
				m_EnableDebug = false;
			if (m_EnableSSAO)
				ImGui::Checkbox("Debug SSAO", &m_EnableDebug);
		}
	}
	ImGui::End();
#else
	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	// 渲染模式
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
		m_RenderMode = RenderMode::Basic;
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
		m_RenderMode = RenderMode::NormalMap;
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3))
		m_RenderMode = RenderMode::DisplacementMap;
	if (keyState.IsKeyDown(Keyboard::Q))
		m_FillMode = IEffect::WireFrame;
	else
		m_FillMode = IEffect::Solid;

	// 位移幅度
	if (m_HeightScale > 0 && m_KeyboardTracker.IsKeyPressed(Keyboard::Z))
		m_HeightScale--;
	if (m_HeightScale < 15 && m_KeyboardTracker.IsKeyPressed(Keyboard::X))
		m_HeightScale++;

	// 调试模式开关
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::E))
		m_EnableDebug = !m_EnableDebug;
	// SSAO开关
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::R))
	{
		m_EnableSSAO = !m_EnableSSAO;
		m_pBasicEffect->SetSSAOEnabled(m_EnableSSAO);
	}
	// 灰度模式开关
	if (m_EnableSSAO && m_KeyboardTracker.IsKeyPressed(Keyboard::G))
		m_GrayMode = !m_GrayMode;

	// ******************
	// 自由摄像机的操作
	//

	// 方向移动
	if (keyState.IsKeyDown(Keyboard::W))
		cam1st->MoveForward(dt * 6.0f);
	if (keyState.IsKeyDown(Keyboard::S))
		cam1st->MoveForward(dt * -6.0f);
	if (keyState.IsKeyDown(Keyboard::A))
		cam1st->Strafe(dt * -6.0f);
	if (keyState.IsKeyDown(Keyboard::D))
		cam1st->Strafe(dt * 6.0f);

	// 在鼠标没进入窗口前仍为ABSOLUTE模式
	if (mouseState.positionMode == Mouse::MODE_RELATIVE)
	{
		cam1st->Pitch(mouseState.y * 0.002f);
		cam1st->RotateY(mouseState.x * 0.002f);
	}

	// 退出程序，这里应向窗口发送销毁信息
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
#endif

	m_pBasicEffect->SetHeightScale(m_HeightScale / 100.0f);
	m_pShadowEffect->SetHeightScale(m_HeightScale / 100.0f);
	m_pSSAOEffect->SetHeightScale(m_HeightScale / 100.0f);

	m_pBasicEffect->SetViewMatrix(m_pCamera->GetViewXM());
	m_pSSAOEffect->SetViewMatrix(m_pCamera->GetViewXM());

	m_pBasicEffect->SetEyePos(m_pCamera->GetPosition());
	m_pSSAOEffect->SetEyePos(m_pCamera->GetPosition());
	m_pShadowEffect->SetEyePos(m_pCamera->GetPosition());

	// 更新光照
	static float theta = 0;
	theta += dt * XM_2PI / 40.0f;

	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR dirVec = XMLoadFloat3(&m_OriginalLightDirs[i]);
		dirVec = XMVector3Transform(dirVec, XMMatrixRotationY(theta));
		XMStoreFloat3(&m_DirLights[i].direction, dirVec);
		m_pBasicEffect->SetDirLight(i, m_DirLights[i]);
	}

	//
	// 投影区域为正方体，以原点为中心，以方向光为+Z朝向
	//
	XMVECTOR dirVec = XMLoadFloat3(&m_DirLights[0].direction);
	XMMATRIX LightView = XMMatrixLookAtLH(dirVec * 20.0f * (-2.0f), g_XMZero, g_XMIdentityR1);
	m_pShadowEffect->SetViewMatrix(LightView);

	// 将NDC空间 [-1, +1]^2 变换到纹理坐标空间 [0, 1]^2
	static XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	// S = V * P * T
	m_pBasicEffect->SetShadowTransformMatrix(LightView * XMMatrixOrthographicLH(40.0f, 40.0f, 20.0f, 60.0f) * T);

}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	// ******************
	// 绘制到SSAO图
	//
	if (m_EnableSSAO && m_FillMode == IEffect::Solid)
	{
		m_pSSAOMap->Begin(m_pd3dImmediateContext.Get(), m_pDepthStencilView.Get());
		{
			// 绘制场景到法向量/深度缓冲区
			DrawScene(m_pSSAOEffect.get(), m_RenderMode);

			// 计算环境光遮蔽值到SSAO图
			m_pSSAOMap->ComputeSSAO(m_pd3dImmediateContext.Get(), *m_pSSAOEffect, *m_pCamera);

			// 进行模糊
			m_pSSAOMap->BlurAmbientMap(m_pd3dImmediateContext.Get(), *m_pSSAOEffect, 4);
		}
		m_pSSAOMap->End(m_pd3dImmediateContext.Get());
	}
	
	// ******************
	// 绘制到阴影贴图
	m_pShadowMap->Begin(m_pd3dImmediateContext.Get(), nullptr);
	{
		DrawScene(m_pShadowEffect.get(), m_RenderMode);
	}
	m_pShadowMap->End(m_pd3dImmediateContext.Get());

	// ******************
	// 正常绘制场景
	//
	m_pBasicEffect->SetTextureShadowMap(m_pShadowMap->GetOutputTexture());
	m_pBasicEffect->SetTextureSSAOMap(m_pSSAOMap->GetAmbientTexture());
	m_pBasicEffect->SetSSAOEnabled(m_EnableSSAO && m_FillMode == IEffect::Solid);
	DrawScene(m_pBasicEffect.get(), m_RenderMode);

	// 绘制天空盒
	m_pSkyEffect->SetRenderDefault(m_pd3dImmediateContext.Get());
	m_pDesert->Draw(m_pd3dImmediateContext.Get(), *m_pSkyEffect, *m_pCamera);

	// 解除深度缓冲区绑定
	m_pBasicEffect->SetTextureShadowMap(nullptr);
	m_pBasicEffect->SetTextureSSAOMap(nullptr);
	m_pBasicEffect->Apply(m_pd3dImmediateContext.Get());

#if USE_IMGUI
	if (m_EnableSSAO && m_EnableDebug && m_FillMode == IEffect::Solid)
	{
		if (ImGui::Begin("SSAO Buffer", &m_EnableDebug))
		{
			m_pDebugEffect->SetRenderOneComponentGray(m_pd3dImmediateContext.Get(), 0);
			m_pDebugSSAOMap->Begin(m_pd3dImmediateContext.Get(), Colors::Black);
			{
				m_FullScreenDebugQuad.Draw(m_pd3dImmediateContext.Get(), m_pDebugEffect.get());
			}
			m_pDebugSSAOMap->End(m_pd3dImmediateContext.Get());
			// 解除绑定
			m_pDebugEffect->SetTextureDiffuse(nullptr);
			m_pDebugEffect->Apply(m_pd3dImmediateContext.Get());
			ImVec2 winSize = ImGui::GetWindowSize();
			float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
			ImGui::Image(m_pDebugSSAOMap->GetOutputTexture(), ImVec2(smaller * AspectRatio(), smaller));
		}
		ImGui::End();
	}
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#else
	// ******************
	// 调试绘制SSAO图
	if (m_EnableSSAO && m_EnableDebug && m_FillMode == IEffect::Solid)
	{
		if (m_GrayMode)
		{
			m_pDebugEffect->SetRenderOneComponentGray(m_pd3dImmediateContext.Get(), 0);
		}
		else
		{
			m_pDebugEffect->SetRenderOneComponent(m_pd3dImmediateContext.Get(), 0);
		}

		m_DebugQuad.Draw(m_pd3dImmediateContext.Get(), m_pDebugEffect.get());
		// 解除绑定
		m_pDebugEffect->SetTextureDiffuse(nullptr);
		m_pDebugEffect->Apply(m_pd3dImmediateContext.Get());
	}


	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式: 自由视角  Esc退出\n";
		text += L"1-基础绘制 2-法线贴图绘制 3-位移贴图绘制 按住Q-线框绘制\n"
			L"当前绘制: ";
		switch (m_RenderMode)
		{
		case GameApp::RenderMode::Basic: text += L"基础绘制\n"; break;
		case GameApp::RenderMode::NormalMap: text += L"法线贴图绘制\n"; break;
		case GameApp::RenderMode::DisplacementMap: text += L"位移贴图绘制\n"; break;
		}
		text += L"HeightScale(-Z/X+): " + std::to_wstring(m_HeightScale / 100.0f);
		text += L"\nSSAO: " + (m_EnableSSAO ? std::wstring(L"开") : std::wstring(L"关")) + L" (R切换)\n";
		if (m_EnableSSAO)
		{
			text += L"调试SSAO图: " + (m_EnableDebug ? std::wstring(L"开") : std::wstring(L"关")) + L" (E切换)\n";
			if (m_EnableDebug)
				text += L"G-灰度/单通道色显示切换\n";
		}
		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}
#endif
	HR(m_pSwapChain->Present(0, 0));
}

void GameApp::DrawScene(BasicEffect* pBasicEffect, RenderMode renderMode)
{
	// 地面和石柱
	switch (renderMode)
	{
	case RenderMode::Basic:
		pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), IEffect::RenderObject, m_FillMode);
		m_Ground.Draw(m_pd3dImmediateContext.Get(), pBasicEffect);

		pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), IEffect::RenderInstance, m_FillMode);
		m_Cylinder.DrawInstanced(m_pd3dImmediateContext.Get(), pBasicEffect, m_CylinderTransforms);
		break;
	case RenderMode::NormalMap:
		pBasicEffect->SetRenderWithNormalMap(m_pd3dImmediateContext.Get(), IEffect::RenderObject, m_FillMode);
		m_GroundT.Draw(m_pd3dImmediateContext.Get(), pBasicEffect);

		pBasicEffect->SetRenderWithNormalMap(m_pd3dImmediateContext.Get(), IEffect::RenderInstance, m_FillMode);
		m_CylinderT.DrawInstanced(m_pd3dImmediateContext.Get(), pBasicEffect, m_CylinderTransforms);
		break;
	case RenderMode::DisplacementMap:
		pBasicEffect->SetRenderWithDisplacementMap(m_pd3dImmediateContext.Get(), IEffect::RenderObject, m_FillMode);
		m_GroundT.Draw(m_pd3dImmediateContext.Get(), pBasicEffect);

		pBasicEffect->SetRenderWithDisplacementMap(m_pd3dImmediateContext.Get(), IEffect::RenderInstance, m_FillMode);
		m_CylinderT.DrawInstanced(m_pd3dImmediateContext.Get(), pBasicEffect, m_CylinderTransforms);
		break;
	}

	// 石球
	pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), IEffect::RenderInstance, m_FillMode);
	m_Sphere.DrawInstanced(m_pd3dImmediateContext.Get(), pBasicEffect, m_SphereTransforms);

	// 房屋
	pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), IEffect::RenderObject);	// 看这个的线框就没必要了吧
	m_House.Draw(m_pd3dImmediateContext.Get(), pBasicEffect);
}

void GameApp::DrawScene(ShadowEffect* pShadowEffect, RenderMode renderMode)
{
	// 地面和石柱
	switch (renderMode)
	{
	case RenderMode::Basic:
	case RenderMode::NormalMap:
		pShadowEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), IEffect::RenderObject);
		m_Ground.Draw(m_pd3dImmediateContext.Get(), pShadowEffect);

		pShadowEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), IEffect::RenderInstance);
		m_Cylinder.DrawInstanced(m_pd3dImmediateContext.Get(), pShadowEffect, m_CylinderTransforms);
		break;
	case RenderMode::DisplacementMap:
		pShadowEffect->SetRenderWithDisplacementMap(m_pd3dImmediateContext.Get(), IEffect::RenderObject);
		m_Ground.Draw(m_pd3dImmediateContext.Get(), pShadowEffect);

		pShadowEffect->SetRenderWithDisplacementMap(m_pd3dImmediateContext.Get(), IEffect::RenderInstance);
		m_Cylinder.DrawInstanced(m_pd3dImmediateContext.Get(), pShadowEffect, m_CylinderTransforms);
		break;
	}
	
	// 石球
	pShadowEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), IEffect::RenderInstance);
	m_Sphere.DrawInstanced(m_pd3dImmediateContext.Get(), pShadowEffect, m_SphereTransforms);

	// 房屋
	pShadowEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), IEffect::RenderObject);
	m_House.Draw(m_pd3dImmediateContext.Get(), pShadowEffect);
}

void GameApp::DrawScene(SSAOEffect* pSSAOEffect, RenderMode renderMode)
{
	// 地面和石柱
	switch (renderMode)
	{
	case RenderMode::Basic:
	case RenderMode::NormalMap:
		pSSAOEffect->SetRenderNormalDepth(m_pd3dImmediateContext.Get(), IEffect::RenderObject);
		m_Ground.Draw(m_pd3dImmediateContext.Get(), pSSAOEffect);

		pSSAOEffect->SetRenderNormalDepth(m_pd3dImmediateContext.Get(), IEffect::RenderInstance);
		m_Cylinder.DrawInstanced(m_pd3dImmediateContext.Get(), pSSAOEffect, m_CylinderTransforms);
		break;
	case RenderMode::DisplacementMap:
		pSSAOEffect->SetRenderNormalDepthWithDisplacementMap(m_pd3dImmediateContext.Get(), IEffect::RenderObject);
		m_Ground.Draw(m_pd3dImmediateContext.Get(), pSSAOEffect);

		pSSAOEffect->SetRenderNormalDepthWithDisplacementMap(m_pd3dImmediateContext.Get(), IEffect::RenderInstance);
		m_Cylinder.DrawInstanced(m_pd3dImmediateContext.Get(), pSSAOEffect, m_CylinderTransforms);
		break;
	}

	// 石球
	pSSAOEffect->SetRenderNormalDepth(m_pd3dImmediateContext.Get(), IEffect::RenderInstance);
	m_Sphere.DrawInstanced(m_pd3dImmediateContext.Get(), pSSAOEffect, m_SphereTransforms);

	// 房屋
	pSSAOEffect->SetRenderNormalDepth(m_pd3dImmediateContext.Get(), IEffect::RenderObject);
	m_House.Draw(m_pd3dImmediateContext.Get(), pSSAOEffect);
}

bool GameApp::InitResource()
{
	// ******************
	// 初始化摄像机
	//

	auto camera = std::make_shared<FirstPersonCamera>();
	m_pCamera = camera;

	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(XMFLOAT3(0.0f, 0.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

	// ******************
	// 初始化阴影贴图、SSAO图和特效
	//
	m_pShadowMap = std::make_unique<TextureRender>();
	HR(m_pShadowMap->InitResource(m_pd3dDevice.Get(), 2048, 2048, true));

	// 开启纹理、阴影、SSAO
	m_pBasicEffect->SetShadowEnabled(true);
	m_pBasicEffect->SetSSAOEnabled(m_EnableSSAO);
	m_pBasicEffect->SetViewMatrix(camera->GetViewXM());
	m_pBasicEffect->SetProjMatrix(camera->GetProjXM());
	m_pBasicEffect->SetHeightScale(0.07f);
	m_pBasicEffect->SetTessInfo(1.0f, 25.0f, 1.0f, 5.0f);

	m_pShadowEffect->SetProjMatrix(XMMatrixOrthographicLH(40.0f, 40.0f, 20.0f, 60.0f));
	m_pShadowEffect->SetHeightScale(0.07f);
	m_pShadowEffect->SetTessInfo(1.0f, 25.0f, 1.0f, 5.0f);

	m_pDebugEffect->SetWorldMatrix(XMMatrixIdentity());
	m_pDebugEffect->SetViewMatrix(XMMatrixIdentity());
	m_pDebugEffect->SetProjMatrix(XMMatrixIdentity());

	m_pSSAOEffect->SetViewMatrix(camera->GetViewXM());
	m_pSSAOEffect->SetProjMatrix(camera->GetProjXM());
	m_pSSAOEffect->SetHeightScale(0.07f);
	m_pSSAOEffect->SetTessInfo(1.0f, 25.0f, 1.0f, 5.0f);


	// ******************
	// 初始化SSAO图
	//

	m_pSSAOMap = std::make_unique<SSAORender>();
	HR(m_pSSAOMap->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, XM_PI / 3, 1000.0f));
	m_pDebugSSAOMap = std::make_unique<TextureRender>();
	HR(m_pDebugSSAOMap->InitResource(m_pd3dDevice.Get(), m_ClientWidth / 2, m_ClientHeight / 2));

	// ******************
	// 初始化对象
	//
	
	ComPtr<ID3D11ShaderResourceView> bricksNormalMap, floorNormalMap, bricksDiffuse, floorDiffuse, stoneDiffuse;
	
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\bricks_nmap.dds", nullptr, bricksNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\floor_nmap.dds", nullptr, floorNormalMap.GetAddressOf()));

	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\bricks.dds", nullptr, bricksDiffuse.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\floor.dds", nullptr, floorDiffuse.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\stone.dds", nullptr, stoneDiffuse.GetAddressOf()));

	// 地面
	{
		Model groundModel;
		groundModel.SetMesh(m_pd3dDevice.Get(), Geometry::CreateTerrain(XMFLOAT2(20.0f, 30.0f), XMUINT2(40, 50), XMFLOAT2(8.0f, 10.0f)));
		groundModel.modelParts[0].material.ambient = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
		groundModel.modelParts[0].material.diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
		groundModel.modelParts[0].material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
		groundModel.modelParts[0].material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		groundModel.modelParts[0].texDiffuse = floorDiffuse;
		groundModel.modelParts[0].texNormalMap = floorNormalMap;	// SSAO_NormalDepth需要用到
		m_Ground.SetModel(std::move(groundModel));
		m_Ground.GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
	}
	
	{
		Model groundTModel;
		groundTModel.SetMesh(m_pd3dDevice.Get(), Geometry::CreateTerrain<VertexPosNormalTangentTex>(XMFLOAT2(20.0f, 30.0f), XMUINT2(40, 50), XMFLOAT2(8.0f, 10.0f)));
		groundTModel.modelParts[0].material.ambient = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
		groundTModel.modelParts[0].material.diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
		groundTModel.modelParts[0].material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
		groundTModel.modelParts[0].material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		groundTModel.modelParts[0].texDiffuse = floorDiffuse;
		groundTModel.modelParts[0].texNormalMap = floorNormalMap;

		m_GroundT.SetModel(std::move(groundTModel));
		m_GroundT.GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
	}
	
	// 圆柱
	{
		Model cylinderModel;
		cylinderModel.SetMesh(m_pd3dDevice.Get(), Geometry::CreateCylinder(0.5f, 3.0f, 15, 15, 1.0f, 2.0f));
		cylinderModel.modelParts[0].material.ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		cylinderModel.modelParts[0].material.diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		cylinderModel.modelParts[0].material.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 32.0f);
		cylinderModel.modelParts[0].material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		cylinderModel.modelParts[0].texDiffuse = bricksDiffuse;
		cylinderModel.modelParts[0].texNormalMap = bricksNormalMap;	// SSAO_NormalDepth需要用到
		m_Cylinder.SetModel(std::move(cylinderModel));
	}
	{
		Model cylinderTModel;
		cylinderTModel.SetMesh(m_pd3dDevice.Get(), Geometry::CreateCylinder<VertexPosNormalTangentTex>(0.5f, 3.0f, 15, 15, 1.0f, 2.0f));
		cylinderTModel.modelParts[0].material.ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		cylinderTModel.modelParts[0].material.diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		cylinderTModel.modelParts[0].material.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 32.0f);
		cylinderTModel.modelParts[0].material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		cylinderTModel.modelParts[0].texDiffuse = bricksDiffuse;
		cylinderTModel.modelParts[0].texNormalMap = bricksNormalMap;
		m_CylinderT.SetModel(std::move(cylinderTModel));
	}

	m_CylinderTransforms.resize(10);
	for (int i = 0; i < 10; ++i)
	{
		m_CylinderTransforms[i].SetPosition(-6.0f + 12.0f * (i / 5), -1.5f, -10.0f + (i % 5) * 5.0f);
	}

	// 石球
	{
		Model sphereModel;
		sphereModel.SetMesh(m_pd3dDevice.Get(), Geometry::CreateSphere(0.5f));
		sphereModel.modelParts[0].material.ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		sphereModel.modelParts[0].material.diffuse = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
		sphereModel.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
		sphereModel.modelParts[0].material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		sphereModel.modelParts[0].texDiffuse = stoneDiffuse;
		m_Sphere.SetModel(std::move(sphereModel));
	}

	m_SphereTransforms.resize(10);
	for (int i = 0; i < 10; ++i)
	{
		m_SphereTransforms[i].SetPosition(-6.0f + 12.0f * (i / 5), 0.5f, -10.0f + (i % 5) * 5.0f);
	}

	// 房屋
	// 修改了mtl文件让其更亮
	ObjReader objReader;
	objReader.Read(L"..\\Model\\house.mbo", L"..\\Model\\house.obj");
	
	m_House.SetModel(Model(m_pd3dDevice.Get(), objReader));

	XMMATRIX S = XMMatrixScaling(0.01f, 0.01f, 0.01f);
	BoundingBox houseBox = m_House.GetLocalBoundingBox();
	houseBox.Transform(houseBox, S);
	
	Transform& houseTransform = m_House.GetTransform();
	houseTransform.SetScale(0.01f, 0.01f, 0.01f);
	houseTransform.SetPosition(0.0f, -(houseBox.Center.y - houseBox.Extents.y + 3.0f), 0.0f);



	// 调试用矩形
	{
		Model quadModel;
		quadModel.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow<VertexPosNormalTex>(XMFLOAT2(0.666666f, 0.666666f), XMFLOAT2(0.333333f, 0.333333f)));
		quadModel.modelParts[0].texDiffuse = m_pSSAOMap->GetAmbientTexture();
		m_DebugQuad.SetModel(std::move(quadModel));
	}
	{
		Model quadModel;
		quadModel.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow<VertexPosNormalTex>());
		quadModel.modelParts[0].texDiffuse = m_pSSAOMap->GetAmbientTexture();
		m_FullScreenDebugQuad.SetModel(std::move(quadModel));
	}

	// ******************
	// 初始化天空盒相关
	//
	m_pDesert = std::make_unique<SkyRender>();
	HR(m_pDesert->InitResource(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		L"..\\Texture\\desertcube1024.dds"));

	m_pBasicEffect->SetTextureCube(m_pDesert->GetTextureCube());

	// ******************
	// 初始化光照
	//
	m_DirLights[0].ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_DirLights[0].diffuse = XMFLOAT4(0.5f, 0.5f, 0.4f, 1.0f);
	m_DirLights[0].specular = XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
	m_DirLights[0].direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	m_DirLights[1].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[1].diffuse = XMFLOAT4(0.40f, 0.40f, 0.40f, 1.0f);
	m_DirLights[1].specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[1].direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

	m_DirLights[2].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[2].diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[2].specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[2].direction = XMFLOAT3(0.0f, 0.0, -1.0f);

	for (int i = 0; i < 3; ++i)
	{
		m_OriginalLightDirs[i] = m_DirLights[i].direction;
		m_pBasicEffect->SetDirLight(i, m_DirLights[i]);
	}
		


	
	// ******************
	// 设置调试对象名
	//
	m_Ground.SetDebugObjectName("Ground");
	m_GroundT.SetDebugObjectName("GroundT");
	m_Cylinder.SetDebugObjectName("Cylinder");
	m_CylinderT.SetDebugObjectName("CylinderT");
	m_Sphere.SetDebugObjectName("Sphere");
	m_SphereT.SetDebugObjectName("SphereT");
	m_House.SetDebugObjectName("House");
	m_DebugQuad.SetDebugObjectName("DebugQuad");
	m_pShadowMap->SetDebugObjectName("ShadowMap");
	m_pSSAOMap->SetDebugObjectName("SSAOMap");
	m_pDesert->SetDebugObjectName("Desert");

	return true;
}

