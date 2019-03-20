#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
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
	RenderStates::InitAll(m_pd3dDevice);

	if (!m_BasicEffect.InitAll(m_pd3dDevice))
		return false;

	if (!m_SkyEffect.InitAll(m_pd3dDevice))
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
		OutputDebugString(L"\n警告：Direct2D与Direct3D互操作性功能受限，你将无法看到文本信息。现提供下述可选方法：\n"
			"1. 对于Win7系统，需要更新至Win7 SP1，并安装KB2670838补丁以支持Direct2D显示。\n"
			"2. 自行完成Direct3D 10.1与Direct2D的交互。详情参阅："
			"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			"3. 使用别的字体库，比如FreeType。\n\n");
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

	// ********************
	// 自由摄像机的操作
	//

	// 方向移动
	if (keyState.IsKeyDown(Keyboard::W))
		cam1st->MoveForward(dt * 3.0f);
	if (keyState.IsKeyDown(Keyboard::S))
		cam1st->MoveForward(dt * -3.0f);
	if (keyState.IsKeyDown(Keyboard::A))
		cam1st->Strafe(dt * -3.0f);
	if (keyState.IsKeyDown(Keyboard::D))
		cam1st->Strafe(dt * 3.0f);

	// 视野旋转，防止开始的差值过大导致的突然旋转
	cam1st->Pitch(mouseState.y * dt * 1.25f);
	cam1st->RotateY(mouseState.x * dt * 1.25f);

	// 更新观察矩阵
	m_pCamera->UpdateViewMatrix();
	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPositionXM());

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
	m_pDaylight->Cache(m_pd3dImmediateContext, m_BasicEffect);

	// 绘制动态天空盒的每个面（以球体为中心）
	for (int i = 0; i < 6; ++i)
	{
		m_pDaylight->BeginCapture(
			m_pd3dImmediateContext, m_BasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i));

		// 不绘制中心球
		DrawScene(false);
	}

	// 恢复之前的绘制设定
	m_pDaylight->Restore(m_pd3dImmediateContext, m_BasicEffect, *m_pCamera);
	
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
			"鼠标移动控制视野 W/S/A/D移动\n"
			"1-法线贴图: ";
		text += m_EnableNormalMap ? L"开启\n" : L"关闭\n";
		text += L"切换纹理: 2-地板  3-鹅卵石面\n"
			"当前纹理: ";
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
	m_EnableNormalMap = true;

	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\bricks_nmap.dds", nullptr, m_BricksNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\floor_nmap.dds", nullptr, m_FloorNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\stones_nmap.dds", nullptr, m_StonesNormalMap.GetAddressOf()));

	// ******************
	// 初始化天空盒相关

	m_pDaylight = std::make_unique<DynamicSkyRender>(
		m_pd3dDevice, m_pd3dImmediateContext, 
		L"Texture\\daylight.jpg", 
		5000.0f, 256);

	m_BasicEffect.SetTextureCube(m_pDaylight->GetDynamicTextureCube());

	// ******************
	// 初始化游戏对象
	//

	m_GroundMode = GroundMode::Floor;
	
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\floor.dds", nullptr, m_FloorDiffuse.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\stones.dds", nullptr, m_StonesDiffuse.GetAddressOf()));
	// 地面
	m_GroundModel.SetMesh(m_pd3dDevice,
		Geometry::CreatePlane(XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	m_GroundModel.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_GroundModel.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_GroundModel.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_GroundModel.modelParts[0].material.Reflect = XMFLOAT4();
	m_GroundModel.modelParts[0].texDiffuse = m_FloorDiffuse;
	m_Ground.SetModel(m_GroundModel);

	// 带切线向量的地面
	m_GroundTModel.SetMesh(m_pd3dDevice, Geometry::CreatePlane<VertexPosNormalTangentTex>(
		XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	m_GroundTModel.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_GroundTModel.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_GroundTModel.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_GroundTModel.modelParts[0].material.Reflect = XMFLOAT4();
	m_GroundTModel.modelParts[0].texDiffuse = m_FloorDiffuse;
	m_GroundT.SetModel(m_GroundTModel);

	// 球体
	Model model;
	ComPtr<ID3D11ShaderResourceView> texDiffuse;

	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(),
		L"Texture\\stone.dds",
		nullptr,
		texDiffuse.GetAddressOf()));

	model.SetMesh(m_pd3dDevice, Geometry::CreateSphere(1.0f, 30, 30));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].texDiffuse = texDiffuse;
	m_Sphere.SetModel(std::move(model));

	// 柱体
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(),
		L"Texture\\bricks.dds",
		nullptr,
		texDiffuse.ReleaseAndGetAddressOf()));

	model.SetMesh(m_pd3dDevice,
		Geometry::CreateCylinder(0.5f, 2.0f));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	model.modelParts[0].texDiffuse = texDiffuse;
	m_Cylinder.SetModel(std::move(model));

	// 带切线向量的柱体
	model.SetMesh(m_pd3dDevice,
		Geometry::CreateCylinder<VertexPosNormalTangentTex>(0.5f, 2.0f));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	model.modelParts[0].texDiffuse = texDiffuse;
	m_CylinderT.SetModel(std::move(model));

	// ******************
	// 初始化摄像机
	//
	m_CameraMode = CameraMode::Free;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	// 初始化并更新观察矩阵、投影矩阵(摄像机将被固定)
	camera->UpdateViewMatrix();
	m_BasicEffect.SetViewMatrix(camera->GetViewXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjXM());


	// ******************
	// 初始化不会变化的值
	//

	// 方向光
	DirectionalLight dirLight[4];
	dirLight[0].Ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
	dirLight[0].Diffuse = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	dirLight[0].Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].Direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	dirLight[1] = dirLight[0];
	dirLight[1].Direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
	dirLight[2] = dirLight[0];
	dirLight[2].Direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
	dirLight[3] = dirLight[0];
	dirLight[3].Direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
	for (int i = 0; i < 4; ++i)
		m_BasicEffect.SetDirLight(i, dirLight[i]);

	return true;
}

void GameApp::DrawScene(bool drawCenterSphere)
{
	// 绘制模型
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext, BasicEffect::RenderObject);
	m_BasicEffect.SetTextureUsed(true);
	
	// 只绘制球体的反射效果
	if (drawCenterSphere)
	{
		m_BasicEffect.SetReflectionEnabled(true);
		m_BasicEffect.SetRefractionEnabled(false);
		m_Sphere.Draw(m_pd3dImmediateContext, m_BasicEffect);
	}
	
	// 绘制地面
	m_BasicEffect.SetReflectionEnabled(false);
	m_BasicEffect.SetRefractionEnabled(false);
	
	if (m_EnableNormalMap)
	{
		m_BasicEffect.SetRenderWithNormalMap(m_pd3dImmediateContext, BasicEffect::RenderObject);
		if (m_GroundMode == GroundMode::Floor)
			m_BasicEffect.SetTextureNormalMap(m_FloorNormalMap);
		else
			m_BasicEffect.SetTextureNormalMap(m_StonesNormalMap);
		m_GroundT.Draw(m_pd3dImmediateContext, m_BasicEffect);
	}
	else
	{
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext, BasicEffect::RenderObject);
		m_Ground.Draw(m_pd3dImmediateContext, m_BasicEffect);
	}

	// 绘制五个圆柱
	// 需要固定位置
	static std::vector<XMMATRIX> cyliderWorlds = {
		XMMatrixTranslation(0.0f, -1.99f, 0.0f),
		XMMatrixTranslation(4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, -4.5f),
		XMMatrixTranslation(4.5f, -1.99f, -4.5f)
	};

	if (m_EnableNormalMap)
	{
		m_BasicEffect.SetRenderWithNormalMap(m_pd3dImmediateContext, BasicEffect::RenderInstance);
		m_BasicEffect.SetTextureNormalMap(m_BricksNormalMap);
		m_CylinderT.DrawInstanced(m_pd3dImmediateContext, m_BasicEffect, cyliderWorlds);
	}
	else
	{
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext, BasicEffect::RenderInstance);
		m_Cylinder.DrawInstanced(m_pd3dImmediateContext, m_BasicEffect, cyliderWorlds);
	}
	
	// 绘制五个圆球
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext, BasicEffect::RenderInstance);
	static float rad = 0.0f;
	rad += 0.001f;
	// 需要动态位置，不使用static
	std::vector<XMMATRIX> sphereWorlds = {
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(4.5f, 0.5f * XMScalarSin(rad), 4.5f),
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(-4.5f, 0.5f * XMScalarSin(rad), 4.5f),
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(-4.5f, 0.5f * XMScalarSin(rad), -4.5f),
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(4.5f, 0.5f * XMScalarSin(rad), -4.5f),
		XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(2.5f * XMScalarCos(rad), 0.0f, 2.5f * XMScalarSin(rad))
	};
	m_Sphere.DrawInstanced(m_pd3dImmediateContext, m_BasicEffect, sphereWorlds);

	// 绘制天空盒
	m_SkyEffect.SetRenderDefault(m_pd3dImmediateContext);

	if (drawCenterSphere)
		m_pDaylight->Draw(m_pd3dImmediateContext, m_SkyEffect, *m_pCamera);
	else
		m_pDaylight->Draw(m_pd3dImmediateContext, m_SkyEffect, m_pDaylight->GetCamera());
	
}

