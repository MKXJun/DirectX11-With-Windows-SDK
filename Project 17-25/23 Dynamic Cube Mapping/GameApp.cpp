#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_CameraMode(CameraMode::Free),
	m_Eta(1.0f / 1.51f),
	m_SkyBoxMode(SkyBoxMode::Daylight),
	m_SphereMode(SphereMode::Reflection),
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

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_SkyEffect.InitAll(m_pd3dDevice.Get()))
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
		m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
	}
}

void GameApp::UpdateScene(float dt)
{
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();

	// 第一人称/自由摄像机的操作
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

	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

	if (ImGui::Begin("Dynamic Cube Mapping"))
	{
		static int skybox_item = 0;
		const char* skybox_modes[] = {
			"Daylight",
			"Sunset",
			"Desert"
		};
		if (ImGui::Combo("Skybox", &skybox_item, skybox_modes, ARRAYSIZE(skybox_modes)))
		{
			m_SkyBoxMode = static_cast<SkyBoxMode>(skybox_item);
			switch (m_SkyBoxMode)
			{
			case GameApp::SkyBoxMode::Daylight:
				m_BasicEffect.SetTextureCube(m_pDaylight->GetTextureCube());
				break;
			case GameApp::SkyBoxMode::Sunset:
				m_BasicEffect.SetTextureCube(m_pSunset->GetTextureCube());
				break;
			case GameApp::SkyBoxMode::Desert:
				m_BasicEffect.SetTextureCube(m_pDesert->GetTextureCube());
				break;
			}
		}
		static int sphere_item = static_cast<int>(m_SphereMode);
		static const char* sphere_modes[] = {
			"None",
			"Reflection",
			"Refraction"
		};
		if (ImGui::Combo("Sphere Mode", &sphere_item, sphere_modes, ARRAYSIZE(sphere_modes)))
		{
			m_SphereMode = static_cast<SphereMode>(sphere_item);
		}
		if (m_SphereMode == SphereMode::Refraction)
		{
			if (ImGui::SliderFloat("Eta", &m_Eta, 0.2f, 1.0f))
			{
				m_BasicEffect.SetRefractionEta(m_Eta);
			}
		}
	}
	ImGui::End();
	ImGui::Render();

#else
	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

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

	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

	// 选择天空盒
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		m_SkyBoxMode = SkyBoxMode::Daylight;
		m_BasicEffect.SetTextureCube(m_pDaylight->GetTextureCube());
	}
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_SkyBoxMode = SkyBoxMode::Sunset;
		m_BasicEffect.SetTextureCube(m_pSunset->GetTextureCube());
	}
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		m_SkyBoxMode = SkyBoxMode::Desert;
		m_BasicEffect.SetTextureCube(m_pDesert->GetTextureCube());
	}

	// 选择球的渲染模式
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D4))
	{
		m_SphereMode = SphereMode::None;
	}
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D5))
	{
		m_SphereMode = SphereMode::Reflection;
	}
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D6))
	{
		m_SphereMode = SphereMode::Refraction;
	}
	
	// 滚轮调整折射率
	m_Eta += mouseState.scrollWheelValue / 12000.0f;
	if (m_Eta > 1.0f)
	{
		m_Eta = 1.0f;
	}
	else if (m_Eta <= 0.2f)
	{
		m_Eta = 0.2f;
	}
	m_BasicEffect.SetRefractionEta(m_Eta);

	// 重置滚轮值
	m_pMouse->ResetScrollWheelValue();

	// 退出程序，这里应向窗口发送销毁信息
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
#endif

	// 设置球体动画速度
	m_SphereRad += 2.0f * dt;
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	// ******************
	// 生成动态天空盒
	//

	// 保留当前绘制的渲染目标视图和深度模板视图
	switch (m_SkyBoxMode)
	{
	case SkyBoxMode::Daylight: m_pDaylight->Cache(m_pd3dImmediateContext.Get(), m_BasicEffect); break;
	case SkyBoxMode::Sunset: m_pSunset->Cache(m_pd3dImmediateContext.Get(), m_BasicEffect); break;
	case SkyBoxMode::Desert: m_pDesert->Cache(m_pd3dImmediateContext.Get(), m_BasicEffect); break;
	}

	// 绘制动态天空盒的每个面（以球体为中心）
	for (int i = 0; i < 6; ++i)
	{
		switch (m_SkyBoxMode)
		{
		case SkyBoxMode::Daylight: m_pDaylight->BeginCapture(
			m_pd3dImmediateContext.Get(), m_BasicEffect, XMFLOAT3(), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		case SkyBoxMode::Sunset: m_pSunset->BeginCapture(
			m_pd3dImmediateContext.Get(), m_BasicEffect, XMFLOAT3(), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		case SkyBoxMode::Desert: m_pDesert->BeginCapture(
			m_pd3dImmediateContext.Get(), m_BasicEffect, XMFLOAT3(), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		}

		// 不绘制中心球
		DrawScene(false);
	}

	// 恢复之前的绘制设定
	switch (m_SkyBoxMode)
	{
	case SkyBoxMode::Daylight: m_pDaylight->Restore(m_pd3dImmediateContext.Get(), m_BasicEffect, *m_pCamera); break;
	case SkyBoxMode::Sunset: m_pSunset->Restore(m_pd3dImmediateContext.Get(), m_BasicEffect, *m_pCamera); break;
	case SkyBoxMode::Desert: m_pDesert->Restore(m_pd3dImmediateContext.Get(), m_BasicEffect, *m_pCamera); break;
	}
	
	// ******************
	// 绘制场景
	//

	// 预先清空
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 绘制中心球
	DrawScene(true);
	

#ifdef USE_IMGUI
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#else
	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式: 自由视角  Esc退出\n"
			L"鼠标移动控制视野 滚轮调整折射率 W/S/A/D移动\n"
			L"切换天空盒: 1-白天 2-日落 3-沙漠\n"
			L"中心球模式: 4-无   5-反射 6-折射\n"
			L"当前天空盒: ";

		switch (m_SkyBoxMode)
		{
		case SkyBoxMode::Daylight: text += L"白天"; break;
		case SkyBoxMode::Sunset: text += L"日落"; break;
		case SkyBoxMode::Desert: text += L"沙漠"; break;
		}

		text += L" 当前模式: ";
		switch (m_SphereMode)
		{
		case SphereMode::None: text += L"无"; break;
		case SphereMode::Reflection: text += L"反射"; break;
		case SphereMode::Refraction: text += L"折射\n折射率: " + std::to_wstring(m_Eta); break;
		}

		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}
#endif

	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// 初始化天空盒相关

	m_pDaylight = std::make_unique<DynamicSkyRender>();
	HR(m_pDaylight->InitResource(
		m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		L"..\\Texture\\daylight.jpg", 
		256));

	m_pSunset = std::make_unique<DynamicSkyRender>();
	HR(m_pSunset->InitResource(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		std::vector<std::wstring>{
		L"..\\Texture\\sunset_posX.bmp", L"..\\Texture\\sunset_negX.bmp",
		L"..\\Texture\\sunset_posY.bmp", L"..\\Texture\\sunset_negY.bmp", 
		L"..\\Texture\\sunset_posZ.bmp", L"..\\Texture\\sunset_negZ.bmp", },
		256));

	m_pDesert = std::make_unique<DynamicSkyRender>();
	HR(m_pDesert->InitResource(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		L"..\\Texture\\desertcube1024.dds",
		256));

	m_BasicEffect.SetTextureCube(m_pDaylight->GetDynamicTextureCube());

	// ******************
	// 初始化游戏对象
	//
	
	// 球体
	{
		Model model;
		model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateSphere(1.0f, 30, 30));
		model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		model.modelParts[0].material.diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		model.modelParts[0].material.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		model.modelParts[0].material.reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(),
			L"..\\Texture\\stone.dds",
			nullptr,
			model.modelParts[0].texDiffuse.GetAddressOf()));
		m_Sphere.SetModel(std::move(model));
		m_Sphere.ResizeBuffer(m_pd3dDevice.Get(), 5);
	}
	// 地面
	{
		Model model;
		model.SetMesh(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(10.0f, 10.0f), XMFLOAT2(5.0f, 5.0f)));
		model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
		model.modelParts[0].material.reflect = XMFLOAT4();
		HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(),
			L"..\\Texture\\floor.dds",
			nullptr,
			model.modelParts[0].texDiffuse.GetAddressOf()));
		m_Ground.SetModel(std::move(model));
		m_Ground.GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
	}
	// 柱体
	{
		Model model;
		model.SetMesh(m_pd3dDevice.Get(),
			Geometry::CreateCylinder(0.5f, 2.0f));
		model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
		model.modelParts[0].material.reflect = XMFLOAT4();
		HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(),
			L"..\\Texture\\bricks.dds",
			nullptr,
			model.modelParts[0].texDiffuse.GetAddressOf()));
		m_Cylinder.SetModel(std::move(model));
		m_Cylinder.ResizeBuffer(m_pd3dDevice.Get(), 5);
	}
	

	// ******************
	// 初始化摄像机
	//
	m_CameraMode = CameraMode::Free;
	auto camera = std::make_shared<FirstPersonCamera>();
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(XMFLOAT3(0.0f, 0.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

	m_BasicEffect.SetViewMatrix(camera->GetViewXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjXM());


	// ******************
	// 初始化不会变化的值
	//

	// 方向光
	DirectionalLight dirLight[4];
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

	// ******************
	// 设置调试对象名
	//
	m_Cylinder.SetDebugObjectName("Cylinder");
	m_Ground.SetDebugObjectName("Ground");
	m_Sphere.SetDebugObjectName("Sphere");
	m_pDaylight->SetDebugObjectName("DayLight");
	m_pSunset->SetDebugObjectName("Sunset");
	m_pDesert->SetDebugObjectName("Desert");

	return true;
}

void GameApp::DrawScene(bool drawCenterSphere)
{
	// 绘制模型
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	
	// 只有球体才有反射或折射效果
	if (drawCenterSphere)
	{
		switch (m_SphereMode)
		{
		case SphereMode::None: 
			m_BasicEffect.SetReflectionEnabled(false);
			m_BasicEffect.SetRefractionEnabled(false);
			break;
		case SphereMode::Reflection:
			m_BasicEffect.SetReflectionEnabled(true);
			m_BasicEffect.SetRefractionEnabled(false);
			break;
		case SphereMode::Refraction:
			m_BasicEffect.SetReflectionEnabled(false);
			m_BasicEffect.SetRefractionEnabled(true);
			break;
		}
		m_Sphere.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	}
	
	// 绘制地面
	m_BasicEffect.SetReflectionEnabled(false);
	m_BasicEffect.SetRefractionEnabled(false);
	m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	// 绘制五个圆柱
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
	// 需要固定位置
	static std::vector<Transform> cyliderWorlds = {
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(0.0f, -1.99f, 0.0f)),
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(4.5f, -1.99f, 4.5f)),
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(-4.5f, -1.99f, 4.5f)),
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(-4.5f, -1.99f, -4.5f)),
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(4.5f, -1.99f, -4.5f)),
	};
	m_Cylinder.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, cyliderWorlds);
	
	// 绘制五个圆球
	std::vector<Transform> sphereWorlds = {
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(4.5f, 0.5f * XMScalarSin(m_SphereRad), 4.5f)),
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(-4.5f, 0.5f * XMScalarSin(m_SphereRad), 4.5f)),
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(-4.5f, 0.5f * XMScalarSin(m_SphereRad), -4.5f)),
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(4.5f, 0.5f * XMScalarSin(m_SphereRad), -4.5f)),
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(2.5f * XMScalarCos(m_SphereRad), 0.0f, 2.5f * XMScalarSin(m_SphereRad))),
	};

	m_Sphere.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, sphereWorlds);

	// 绘制天空盒
	m_SkyEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
	switch (m_SkyBoxMode)
	{
	case SkyBoxMode::Daylight: m_pDaylight->Draw(m_pd3dImmediateContext.Get(), m_SkyEffect,
		(drawCenterSphere ? *m_pCamera : m_pDaylight->GetCamera())); break;
	case SkyBoxMode::Sunset: m_pSunset->Draw(m_pd3dImmediateContext.Get(), m_SkyEffect,
		(drawCenterSphere ? *m_pCamera : m_pSunset->GetCamera())); break;
	case SkyBoxMode::Desert: m_pDesert->Draw(m_pd3dImmediateContext.Get(), m_SkyEffect,
		(drawCenterSphere ? *m_pCamera : m_pDesert->GetCamera())); break;
	}
	
}

