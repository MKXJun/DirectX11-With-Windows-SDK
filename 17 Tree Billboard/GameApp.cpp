#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
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
	RenderStates::InitAll(m_pd3dDevice);

	if (!m_BasicEffect.InitAll(m_pd3dDevice))
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

	// 获取子类
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

	if (m_CameraMode == CameraMode::Free)
	{
		// ******************
		// 自由摄像机的操作
		//

		// 方向移动
		if (keyState.IsKeyDown(Keyboard::W))
		{
			cam1st->MoveForward(dt * 3.0f);
		}
		if (keyState.IsKeyDown(Keyboard::S))
		{
			cam1st->MoveForward(dt * -3.0f);
		}
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		// 视野旋转，防止开始的差值过大导致的突然旋转
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);
	}

	// ******************
	// 更新摄像机
	//

	// 将位置限制在[-49.9f, 49.9f]的区域内
	// 不允许穿地
	XMFLOAT3 adjustedPos;
	XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), 
		XMVectorSet(-49.9f, 0.0f, -49.9f, 0.0f), XMVectorSet(49.9f, 99.9f, 49.9f, 0.0f)));
	cam1st->SetPosition(adjustedPos);

	// 更新观察矩阵
	m_pCamera->UpdateViewMatrix();
	m_BasicEffect.SetEyePos(m_pCamera->GetPositionXM());
	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());

	// ******************
	// 开关雾效
	//
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		m_FogEnabled = !m_FogEnabled;
		m_BasicEffect.SetFogState(m_FogEnabled);
	}

	// ******************
	// 白天/黑夜变换
	//
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_IsNight = !m_IsNight;
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
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		m_EnableAlphaToCoverage = !m_EnableAlphaToCoverage;
	}

	// ******************
	// 调整雾的范围
	//
	if (mouseState.scrollWheelValue != 0)
	{
		// 一次滚轮滚动的最小单位为120
		m_FogRange += mouseState.scrollWheelValue / 120;
		if (m_FogRange < 15.0f)
			m_FogRange = 15.0f;
		else if (m_FogRange > 175.0f)
			m_FogRange = 175.0f;
		m_BasicEffect.SetFogRange(m_FogRange);
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
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext);
	m_Ground.Draw(m_pd3dImmediateContext, m_BasicEffect);

	// 绘制树
	m_BasicEffect.SetRenderBillboard(m_pd3dImmediateContext, m_EnableAlphaToCoverage);
	m_BasicEffect.SetMaterial(m_TreeMat);
	UINT stride = sizeof(VertexPosSize);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, mPointSpritesBuffer.GetAddressOf(), &stride, &offset);
	m_BasicEffect.Apply(m_pd3dImmediateContext);
	m_pd3dImmediateContext->Draw(16, 0);

	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"1-雾效开关 2-白天/黑夜雾效切换 3-AlphaToCoverage开关 Esc-退出\n"
			"滚轮-调整雾效范围\n"
			"仅支持自由视角摄像机\n";
		text += std::wstring(L"AlphaToCoverage状态: ") + (m_EnableAlphaToCoverage ? L"开启\n" : L"关闭\n");
		text += std::wstring(L"雾效状态: ") + (m_FogEnabled ? L"开启\n" : L"关闭\n");
		if (m_FogEnabled)
		{
			text += std::wstring(L"天气情况: ") + (m_IsNight ? L"黑夜\n" : L"白天\n");
			text += L"雾效范围: " + std::to_wstring(m_IsNight ? 5 : 15) + L"-" +
				std::to_wstring((m_IsNight ? 5 : 15) + (int)m_FogRange);
		}


		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));

}



bool GameApp::InitResource()
{
	// ******************
	// 初始化各种物体
	//

	// 初始化树纹理资源
	ComPtr<ID3D11Texture2D> test;
	HR(CreateDDSTexture2DArrayFromFile(
		m_pd3dDevice.Get(),
		m_pd3dImmediateContext.Get(),
		std::vector<std::wstring>{
			L"Texture\\tree0.dds",
			L"Texture\\tree1.dds",
			L"Texture\\tree2.dds",
			L"Texture\\tree3.dds"},
		test.GetAddressOf(),
		mTreeTexArray.GetAddressOf()));
	m_BasicEffect.SetTextureArray(mTreeTexArray);

	// 初始化点精灵缓冲区
	InitPointSpritesBuffer();

	// 初始化树的材质
	m_TreeMat.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_TreeMat.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_TreeMat.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	ComPtr<ID3D11ShaderResourceView> texture;
	// 初始化地板
	m_Ground.SetBuffer(m_pd3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, -5.0f, 0.0f), XMFLOAT2(100.0f, 100.0f), XMFLOAT2(10.0f, 10.0f)));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\Grass.dds", nullptr, texture.GetAddressOf()));
	m_Ground.SetTexture(texture);
	Material material;
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
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetPosition(XMFLOAT3());
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();

	m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	m_BasicEffect.SetViewMatrix(camera->GetViewXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjXM());
	m_BasicEffect.SetEyePos(camera->GetPositionXM());

	// ******************
	// 初始化雾效和天气等
	//

	m_BasicEffect.SetFogState(m_FogEnabled);
	m_BasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
	m_BasicEffect.SetFogStart(15.0f);
	m_BasicEffect.SetFogRange(75.0f);

	
	
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
