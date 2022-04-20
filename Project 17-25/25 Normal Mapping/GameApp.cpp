#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_CameraMode(CameraMode::Free),
	m_GroundMode(GroundMode::Floor),
	m_EnableNormalMap(true),
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

	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

	return true;
}

void GameApp::OnResize()
{
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);
	// 释放D2D的相关资源
	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

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

	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

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

	// 法线贴图开关
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
		m_EnableNormalMap = !m_EnableNormalMap;
		
	// 切换地面纹理
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2) && m_GroundMode != GroundMode::Floor)
	{
		m_GroundMode = GroundMode::Floor;
		m_GroundModel.modelParts[0].texDiffuse = m_FloorDiffuse;
		m_GroundTModel.modelParts[0].texDiffuse = m_FloorDiffuse;
		m_Ground.SetModel(m_GroundModel);
		m_GroundT.SetModel(m_GroundTModel);
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3) && m_GroundMode != GroundMode::Stones)
	{
		m_GroundMode = GroundMode::Stones;
		m_GroundModel.modelParts[0].texDiffuse = m_StonesDiffuse;
		m_GroundTModel.modelParts[0].texDiffuse = m_StonesDiffuse;
		m_Ground.SetModel(m_GroundModel);
		m_GroundT.SetModel(m_GroundTModel);
	}

	// 设置球体动画速度
	m_SphereRad += 2.0f * dt;

	// 重置滚轮值
	m_pMouse->ResetScrollWheelValue();

	// 退出程序，这里应向窗口发送销毁信息
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	// ******************
	// 生成动态天空盒
	//

	// 保留当前绘制的渲染目标视图和深度模板视图
	m_pDaylight->Cache(m_pd3dImmediateContext.Get(), m_BasicEffect);

	// 绘制动态天空盒的每个面（以球体为中心）
	for (int i = 0; i < 6; ++i)
	{
		m_pDaylight->BeginCapture(
			m_pd3dImmediateContext.Get(), m_BasicEffect, XMFLOAT3(), static_cast<D3D11_TEXTURECUBE_FACE>(i));

		// 不绘制中心球
		DrawScene(false);
	}

	// 恢复之前的绘制设定
	m_pDaylight->Restore(m_pd3dImmediateContext.Get(), m_BasicEffect, *m_pCamera);
	
	// ******************
	// 绘制场景
	//

	// 预先清空
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 绘制中心球
	DrawScene(true);
	

	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式: 自由视角  Esc退出\n"
			L"鼠标移动控制视野 W/S/A/D移动\n"
			L"1-法线贴图: ";
		text += m_EnableNormalMap ? L"开启\n" : L"关闭\n";
		text += L"切换纹理: 2-地板  3-鹅卵石面\n"
			L"当前纹理: ";
		text += (m_GroundMode == GroundMode::Floor ? L"地板" : L"鹅卵石面");

		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// 初始化法线贴图相关
	//
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\bricks_nmap.dds", nullptr, m_BricksNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\floor_nmap.dds", nullptr, m_FloorNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\stones_nmap.dds", nullptr, m_StonesNormalMap.GetAddressOf()));

	// ******************
	// 初始化天空盒相关

	m_pDaylight = std::make_unique<DynamicSkyRender>();
	HR(m_pDaylight->InitResource(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		L"..\\Texture\\daylight.jpg", 
		256));

	m_BasicEffect.SetTextureCube(m_pDaylight->GetDynamicTextureCube());

	// ******************
	// 初始化游戏对象
	//
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\floor.dds", nullptr, m_FloorDiffuse.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\stones.dds", nullptr, m_StonesDiffuse.GetAddressOf()));
	// 地面
	m_GroundModel.SetMesh(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	m_GroundModel.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_GroundModel.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_GroundModel.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_GroundModel.modelParts[0].material.reflect = XMFLOAT4();
	m_GroundModel.modelParts[0].texDiffuse = m_FloorDiffuse;
	m_Ground.SetModel(m_GroundModel);
	m_Ground.GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
	// 带切线向量的地面
	m_GroundTModel.SetMesh(m_pd3dDevice.Get(), Geometry::CreatePlane<VertexPosNormalTangentTex>(XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	m_GroundTModel.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_GroundTModel.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_GroundTModel.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_GroundTModel.modelParts[0].material.reflect = XMFLOAT4();
	m_GroundTModel.modelParts[0].texDiffuse = m_FloorDiffuse;
	m_GroundT.SetModel(m_GroundTModel);
	m_GroundT.GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
	
	ComPtr<ID3D11ShaderResourceView> texDiffuse;
	// 球体
	{
		Model model;
		HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(),
			L"..\\Texture\\stone.dds",
			nullptr,
			texDiffuse.GetAddressOf()));

		model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateSphere(1.0f, 30, 30));
		model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		model.modelParts[0].material.diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		model.modelParts[0].material.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		model.modelParts[0].material.reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		model.modelParts[0].texDiffuse = texDiffuse;
		m_Sphere.SetModel(std::move(model));
		m_Sphere.ResizeBuffer(m_pd3dDevice.Get(), 5);
	}
	// 柱体
	{
		Model model;
		HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(),
			L"..\\Texture\\bricks.dds",
			nullptr,
			texDiffuse.ReleaseAndGetAddressOf()));

		model.SetMesh(m_pd3dDevice.Get(),
			Geometry::CreateCylinder(0.5f, 2.0f));
		model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
		model.modelParts[0].material.reflect = XMFLOAT4();
		model.modelParts[0].texDiffuse = texDiffuse;
		m_Cylinder.SetModel(std::move(model));
		m_Cylinder.ResizeBuffer(m_pd3dDevice.Get(), 5);
	}
	// 带切线向量的柱体
	{
		Model model;
		model.SetMesh(m_pd3dDevice.Get(),
			Geometry::CreateCylinder<VertexPosNormalTangentTex>(0.5f, 2.0f));
		model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
		model.modelParts[0].material.reflect = XMFLOAT4();
		model.modelParts[0].texDiffuse = texDiffuse;
		m_CylinderT.SetModel(std::move(model));
		m_CylinderT.ResizeBuffer(m_pd3dDevice.Get(), 5);
	}
	

	// ******************
	// 初始化摄像机
	//
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
	m_CylinderT.SetDebugObjectName("CylinderT");
	m_Ground.SetDebugObjectName("Ground");
	m_GroundT.SetDebugObjectName("GroundT");
	m_Sphere.SetDebugObjectName("Sphere");
	m_pDaylight->SetDebugObjectName("DayLight");
	

	return true;
}

void GameApp::DrawScene(bool drawCenterSphere)
{
	// 绘制模型
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	
	// 只绘制球体的反射效果
	if (drawCenterSphere)
	{
		m_BasicEffect.SetReflectionEnabled(true);
		m_BasicEffect.SetRefractionEnabled(false);
		m_Sphere.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	}
	
	// 绘制地面
	m_BasicEffect.SetReflectionEnabled(false);
	m_BasicEffect.SetRefractionEnabled(false);
	
	if (m_EnableNormalMap)
	{
		m_BasicEffect.SetRenderWithNormalMap(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
		if (m_GroundMode == GroundMode::Floor)
			m_BasicEffect.SetTextureNormalMap(m_FloorNormalMap.Get());
		else
			m_BasicEffect.SetTextureNormalMap(m_StonesNormalMap.Get());
		m_GroundT.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	}
	else
	{
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
		m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	}

	// 绘制五个圆柱
	// 需要固定位置
	static std::vector<Transform> cyliderWorlds = {
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(0.0f, -1.99f, 0.0f)),
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(4.5f, -1.99f, 4.5f)),
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(-4.5f, -1.99f, 4.5f)),
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(-4.5f, -1.99f, -4.5f)),
		Transform(XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(), XMFLOAT3(4.5f, -1.99f, -4.5f))
	};

	if (m_EnableNormalMap)
	{
		m_BasicEffect.SetRenderWithNormalMap(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
		m_BasicEffect.SetTextureNormalMap(m_BricksNormalMap.Get());
		m_CylinderT.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, cyliderWorlds);
	}
	else
	{
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
		m_Cylinder.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, cyliderWorlds);
	}
	
	// 绘制五个圆球
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);

	std::vector<Transform> sphereWorlds = {
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(4.5f, 0.5f * XMScalarSin(m_SphereRad), 4.5f)),
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(-4.5f, 0.5f * XMScalarSin(m_SphereRad), 4.5f)),
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(-4.5f, 0.5f * XMScalarSin(m_SphereRad), -4.5f)),
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(4.5f, 0.5f * XMScalarSin(m_SphereRad), -4.5f)),
		Transform(XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(), XMFLOAT3(2.5f * XMScalarCos(m_SphereRad), 0.0f, 2.5f * XMScalarSin(m_SphereRad)))
	};
	m_Sphere.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, sphereWorlds);

	// 绘制天空盒
	m_SkyEffect.SetRenderDefault(m_pd3dImmediateContext.Get());

	if (drawCenterSphere)
		m_pDaylight->Draw(m_pd3dImmediateContext.Get(), m_SkyEffect, *m_pCamera);
	else
		m_pDaylight->Draw(m_pd3dImmediateContext.Get(), m_SkyEffect, m_pDaylight->GetCamera());
	
}

