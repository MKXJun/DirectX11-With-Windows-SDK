#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance), m_BaseTime(),
	m_EnabledFog(true), m_EnabledOIT(true), m_EnabledNoDepthWrite(true), m_EnableBlurMode(true),
	m_BlurSigma(2.5f), m_BlurRadius(5), m_BlurTimes(1)
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

	// 重新调整各种渲染器和滤波器
	if (m_pTextureRenderMiddle)
	{
		m_pTextureRenderMiddle = std::make_unique<TextureRender>();
		HR(m_pTextureRenderMiddle->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight));
	}

	if (m_pTextureRenderFinal)
	{
		m_pTextureRenderFinal = std::make_unique<TextureRender>();
		HR(m_pTextureRenderFinal->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight));
	}

	if (m_pOITRender)
	{
		m_pOITRender = std::make_unique<OITRender>();
		HR(m_pOITRender->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, 4));
	}

	if (m_pBlurFilter)
	{
		m_pBlurFilter = std::make_unique<BlurFilter>();
		HR(m_pBlurFilter->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight));
	}
	
	if (m_pSobelFilter)
	{
		m_pSobelFilter = std::make_unique<SobelFilter>();
		HR(m_pSobelFilter->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight));
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

	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);

	// ******************
	// 第三人称摄像机的操作
	//

	// 绕物体旋转
	// 在鼠标没进入窗口前仍为ABSOLUTE模式
	if (mouseState.positionMode == Mouse::MODE_RELATIVE)
	{
		cam3rd->RotateX(mouseState.y * dt * 1.25f);
		cam3rd->RotateY(mouseState.x * dt * 1.25f);
		cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
	}

	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPositionXM());
	// 重置滚轮值
	m_pMouse->ResetScrollWheelValue();
	
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		m_EnabledOIT = !m_EnabledOIT;
	}
	// 雾效开关
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_EnabledFog = !m_EnabledFog;
		m_BasicEffect.SetFogState(m_EnabledFog);
	}
	// 深度写入开关
	if (!m_EnabledOIT && m_KeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		m_EnabledNoDepthWrite = !m_EnabledNoDepthWrite;
	}
	// 滤波模式切换
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D4))
	{
		m_EnableBlurMode = !m_EnableBlurMode;
	}

	if (m_EnableBlurMode)
	{
		// 控制模糊半径
		if (m_KeyboardTracker.IsKeyPressed(Keyboard::Q) && m_BlurRadius > 1)
			--m_BlurRadius;
		if (m_KeyboardTracker.IsKeyPressed(Keyboard::W) && m_BlurRadius < 9)
			++m_BlurRadius;

		// 控制Sigma值
		if (m_KeyboardTracker.IsKeyPressed(Keyboard::A) && m_BlurSigma > 1.0f)
			m_BlurSigma -= 0.5f;
		if (m_KeyboardTracker.IsKeyPressed(Keyboard::S) && m_BlurSigma < 20.0f)
			m_BlurSigma += 0.5f;
		// 控制模糊次数
		if (m_KeyboardTracker.IsKeyPressed(Keyboard::Z) && m_BlurTimes > 1)
			--m_BlurTimes;
		if (m_KeyboardTracker.IsKeyPressed(Keyboard::X) && m_BlurTimes < 5)
			++m_BlurTimes;
	}

	m_pBlurFilter->SetGaussianWeights(m_BlurRadius, m_BlurSigma);


	// 每1/4s生成一个随机水波
	if (m_Timer.TotalTime() - m_BaseTime >= 0.25f)
	{
		m_BaseTime += 0.25f;
		m_pGpuWavesRender->Disturb(m_pd3dImmediateContext.Get(), m_RowRange(m_RandEngine), m_ColRange(m_RandEngine),
			m_MagnitudeRange(m_RandEngine));
	}

	// 更新波浪
	m_pGpuWavesRender->Update(m_pd3dImmediateContext.Get(), dt);

	// 退出程序，这里应向窗口发送销毁信息
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	

	
	m_pTextureRenderFinal->Begin(m_pd3dImmediateContext.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	{
		if (m_EnabledOIT)
		{
			m_pTextureRenderMiddle->Begin(m_pd3dImmediateContext.Get(), reinterpret_cast<const float*>(&Colors::Silver));
		}

		// ******************
		// 1. 绘制不透明对象
		//
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
		m_BasicEffect.SetTexTransformMatrix(XMMatrixIdentity());
		m_Land.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

		// ******************
		// 2. 存放透明物体的像素片元
		//
		if (m_EnabledOIT)
		{
			m_pOITRender->BeginDefaultStore(m_pd3dImmediateContext.Get());
		}
		else
		{
			m_pd3dImmediateContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
			m_pd3dImmediateContext->RSSetState(RenderStates::RSNoCull.Get());
			if (m_EnabledNoDepthWrite)
			{
				m_pd3dImmediateContext->OMSetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);
			}
			else
			{
				m_pd3dImmediateContext->OMSetDepthStencilState(nullptr, 0);
			}
		}
		m_RedBox.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
		m_YellowBox.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
		m_pGpuWavesRender->Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
		if (m_EnabledOIT)
		{
			m_pOITRender->EndStore(m_pd3dImmediateContext.Get());
			m_pTextureRenderMiddle->End(m_pd3dImmediateContext.Get());
			// 渲染到后备缓冲区
			m_pOITRender->Draw(m_pd3dImmediateContext.Get(), m_pTextureRenderMiddle->GetOutputTexture());
		}
	}
	m_pTextureRenderFinal->End(m_pd3dImmediateContext.Get());

	// 滤波环节
	if (m_EnableBlurMode)
	{
		// 高斯滤波
		m_pBlurFilter->Execute(m_pd3dImmediateContext.Get(), m_pTextureRenderFinal->GetOutputTexture(),
			nullptr, m_BlurTimes);
		m_BasicEffect.Set2DRenderDefault(m_pd3dImmediateContext.Get());
		m_ScreenBlurQuad.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
		// 撤除绑定的纹理
		m_BasicEffect.SetTextureDiffuse(nullptr);
		m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
	}
	else
	{
		// Sobel滤波
		m_pSobelFilter->Execute(m_pd3dImmediateContext.Get(), m_pTextureRenderFinal->GetOutputTexture());
		m_BasicEffect.SetRenderComposite(m_pd3dImmediateContext.Get());
		m_BasicEffect.SetTextureComposite(m_pSobelFilter->GetOutputTexture());
		m_ScreenSobelQuad.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
		// 撤除绑定的纹理
		m_BasicEffect.SetTextureDiffuse(nullptr);
		m_BasicEffect.SetTextureComposite(nullptr);
		m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
	}


	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式: 第三人称  Esc退出\n"
			L"鼠标移动控制视野 滚轮控制第三人称观察距离\n"
			L"顺序无关透明度: ";
		text += m_EnabledOIT ? L"开  " : L"关  ";
		text += L"(1-切换)\n雾效: ";
		text += m_EnabledFog ? L"开  " : L"关  ";
		text += L"(2-切换)";
		if (!m_EnabledOIT)
		{
			text += L"\n禁止深度写入: ";
			text += m_EnabledNoDepthWrite ? L"开  " : L"关  ";
			text += L"(3-切换)";
		}
		text += L"\n当前滤波模式: ";
		text += m_EnableBlurMode ? L"高斯模糊  " : L"Sobel算子 ";
		text += L"(4-切换)";
		if (m_EnableBlurMode)
		{
			text += L"\n模糊半径(1-9)(Q-减小 W-增大): " + std::to_wstring(m_BlurRadius);
			text += L"\n模糊Sigma(1.0-20.0)(A-减小 S-增大): " + std::to_wstring(m_BlurSigma);
			text += L"\n模糊次数(1-5)(Z-减小 X-增大): " + std::to_wstring(m_BlurTimes);
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
	// 初始化RTT渲染器、OIT渲染器
	//
	m_pTextureRenderMiddle = std::make_unique<TextureRender>();
	HR(m_pTextureRenderMiddle->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight));

	m_pTextureRenderFinal = std::make_unique<TextureRender>();
	HR(m_pTextureRenderFinal->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight));

	m_pOITRender = std::make_unique<OITRender>();
	HR(m_pOITRender->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, 4));

	// ******************
	// 需要先初始化混合滤波器和Sobel滤波器
	//
	m_pBlurFilter = std::make_unique<BlurFilter>();
	HR(m_pBlurFilter->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight));

	m_pSobelFilter = std::make_unique<SobelFilter>();
	HR(m_pSobelFilter->InitResource(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight));

	// ******************
	// 初始化游戏对象
	//
	ComPtr<ID3D11ShaderResourceView> landDiffuse;
	ComPtr<ID3D11ShaderResourceView> redBoxDiffuse;
	ComPtr<ID3D11ShaderResourceView> yellowBoxDiffuse;
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\grass.dds", nullptr, landDiffuse.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\Red.dds", nullptr, redBoxDiffuse.GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\Yellow.dds", nullptr, yellowBoxDiffuse.GetAddressOf()));

	Model model;
	// 地面
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateTerrain(XMFLOAT2(160.0f, 160.0f),
		XMUINT2(50, 50), XMFLOAT2(10.0f, 10.0f),
		[](float x, float z) { return 0.3f*(z * sinf(0.1f * x) + x * cosf(0.1f * z)); },	// 高度函数
		[](float x, float z) { return XMFLOAT3{ -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z), 1.0f,	
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z) }; }));		// 法向量函数
	Material material;
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	material.reflect = XMFLOAT4();
	model.modelParts[0].material = material;
	model.modelParts[0].texDiffuse = landDiffuse;
	m_Land.SetModel(std::move(model));
	m_Land.GetTransform().SetPosition(0.0f, -1.0f, 0.0f);
	// 透明红盒与黄盒
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateBox(8.0f, 8.0f, 8.0f));
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material = material;
	model.modelParts[0].texDiffuse = redBoxDiffuse;
	m_RedBox.SetModel(std::move(model));
	m_RedBox.GetTransform().SetPosition(-6.0f, 2.0f, -4.0f);
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateBox(8.0f, 8.0f, 8.0f));
	model.modelParts[0].material = material;
	model.modelParts[0].texDiffuse = yellowBoxDiffuse;
	m_YellowBox.SetModel(std::move(model));
	m_YellowBox.GetTransform().SetPosition(-2.0f, 1.8f, 0.0f);
	// 屏幕矩形
	model.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow());
	model.modelParts[0].texDiffuse = m_pBlurFilter->GetOutputTexture();
	m_ScreenBlurQuad.SetModel(std::move(model));
	model.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow());
	model.modelParts[0].texDiffuse = m_pTextureRenderFinal->GetOutputTexture();
	m_ScreenSobelQuad.SetModel(std::move(model));

	// ******************
	// 初始化水面波浪
	//

	// 世界矩阵为默认的单位矩阵，故不设置
	m_pGpuWavesRender = std::make_unique<GpuWavesRender>();
	HR(m_pGpuWavesRender->InitResource(m_pd3dDevice.Get(),
		L"..\\Texture\\water2.dds", 256, 256, 5.0f, 5.0f, 0.03f, 0.625f, 2.0f, 0.2f, 0.05f, 0.1f));
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);
	m_pGpuWavesRender->SetMaterial(material);

	// ******************
	// 初始化随机数生成器
	//
	m_RandEngine.seed(std::random_device()());
	m_RowRange = std::uniform_int_distribution<UINT>(5, m_pGpuWavesRender->RowCount() - 5);
	m_ColRange = std::uniform_int_distribution<UINT>(5, m_pGpuWavesRender->ColumnCount() - 5);
	m_MagnitudeRange = std::uniform_real_distribution<float>(0.5f, 1.0f);

	// ******************
	// 初始化摄像机
	//

	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	m_pCamera = camera;

	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetTarget(XMFLOAT3(0.0f, 2.5f, 0.0f));
	camera->SetDistance(20.0f);
	camera->SetDistanceMinMax(10.0f, 90.0f);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->SetRotationX(XM_PIDIV4);

	m_BasicEffect.SetViewMatrix(camera->GetViewXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjXM());
	
	// ******************
	// 初始化不会变化的值
	//

	// 方向光
	DirectionalLight dirLight[3];
	dirLight[0].ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight[0].diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight[0].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
	
	dirLight[1].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	dirLight[1].diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight[1].specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	dirLight[1].direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	
	dirLight[2].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	dirLight[2].diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight[2].specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	dirLight[2].direction = XMFLOAT3(0.0f, -0.707f, -0.707f);
	for (int i = 0; i < 3; ++i)
		m_BasicEffect.SetDirLight(i, dirLight[i]);

	// ******************
	// 初始化雾效和绘制状态
	//

	m_BasicEffect.SetFogState(true);
	m_BasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
	m_BasicEffect.SetFogStart(15.0f);
	m_BasicEffect.SetFogRange(135.0f);
	m_BasicEffect.SetTextureUsed(true);
	
	// ******************
	// 设置调试对象名
	//
	m_Land.SetDebugObjectName("Land");
	m_RedBox.SetDebugObjectName("RedBox");
	m_YellowBox.SetDebugObjectName("YellowBox");
	m_pGpuWavesRender->SetDebugObjectName("GpuWaveRender");
	m_pTextureRenderMiddle->SetDebugObjectName("TextureRenderMiddle");
	m_pTextureRenderFinal->SetDebugObjectName("TextureRenderFinal");
	m_pBlurFilter->SetDebugObjectName("BlurFilter");
	m_pSobelFilter->SetDebugObjectName("SobelFilter");
	

	return true;
}

