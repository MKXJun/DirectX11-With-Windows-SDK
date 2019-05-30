#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_CameraMode(CameraMode::Free),
	m_FadeAmount(0.0f),
	m_FadeSign(1.0f),
	m_FadeUsed(true),	// 开始淡入
	m_PrintScreenStarted(false)
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

	if (!m_ScreenFadeEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_MinimapEffect.InitAll(m_pd3dDevice.Get()))
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
		// 小地图网格模型重设
		m_Minimap.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow(1.0f - 100.0f / m_ClientWidth * 2,  -1.0f + 100.0f / m_ClientHeight * 2,
			100.0f / m_ClientWidth * 2, 100.0f / m_ClientHeight * 2));
		// 屏幕淡入淡出纹理大小重设
		m_pScreenFadeRender = std::make_unique<TextureRender>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, false);
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

	// 更新淡入淡出值，并限制摄像机行动
	if (m_FadeUsed)
	{
		m_FadeAmount += m_FadeSign * dt / 2.0f;	// 2s时间淡入/淡出
		if (m_FadeSign > 0.0f && m_FadeAmount > 1.0f)
		{
			m_FadeAmount = 1.0f;
			m_FadeUsed = false;	// 结束淡入
		}
		else if (m_FadeSign < 0.0f && m_FadeAmount < 0.0f)
		{
			m_FadeAmount = 0.0f;
			SendMessage(MainWnd(), WM_DESTROY, 0, 0);	// 关闭程序
			// 这里不结束淡出是因为发送关闭窗口的消息还要过一会才真正关闭
		}
	}
	else
	{
		// ******************
		// 自由摄像机的操作
		//

		// 方向移动
		if (keyState.IsKeyDown(Keyboard::W))
			cam1st->Walk(dt * 3.0f);
		if (keyState.IsKeyDown(Keyboard::S))
			cam1st->Walk(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		// 视野旋转，防止开始的差值过大导致的突然旋转
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);

		// 限制移动范围
		XMFLOAT3 adjustedPos;
		XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorReplicate(-90.0f), XMVectorReplicate(90.0f)));
		cam1st->SetPosition(adjustedPos);
	}

	// 更新观察矩阵
	m_pCamera->UpdateViewMatrix();
	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPositionXM());
	m_MinimapEffect.SetEyePos(m_pCamera->GetPositionXM());
	
	// 截屏
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
		m_PrintScreenStarted = true;
		
	// 重置滚轮值
	m_pMouse->ResetScrollWheelValue();

	// 退出程序，开始淡出
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
	{
		m_FadeSign = -1.0f;
		m_FadeUsed = true;
	}
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	
	// ******************
	// 绘制Direct3D部分
	//

	// 预先清空后备缓冲区
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	if (m_FadeUsed)
	{
		// 开始淡入/淡出
		m_pScreenFadeRender->Begin(m_pd3dImmediateContext.Get());
	}


	// 绘制主场景
	DrawScene(false);

	// 此处用于小地图和屏幕绘制
	UINT strides[1] = { sizeof(VertexPosTex) };
	UINT offsets[1] = { 0 };
	
	// 小地图特效应用
	m_MinimapEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
	m_MinimapEffect.Apply(m_pd3dImmediateContext.Get());
	// 最后绘制小地图
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_Minimap.modelParts[0].vertexBuffer.GetAddressOf(), strides, offsets);
	m_pd3dImmediateContext->IASetIndexBuffer(m_Minimap.modelParts[0].indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_pd3dImmediateContext->DrawIndexed(6, 0, 0);

	if (m_FadeUsed)
	{
		// 结束淡入/淡出，此时绘制的场景在屏幕淡入淡出渲染的纹理
		m_pScreenFadeRender->End(m_pd3dImmediateContext.Get());

		// 屏幕淡入淡出特效应用
		m_ScreenFadeEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
		m_ScreenFadeEffect.SetFadeAmount(m_FadeAmount);
		m_ScreenFadeEffect.SetTexture(m_pScreenFadeRender->GetOutputTexture());
		m_ScreenFadeEffect.SetWorldViewProjMatrix(XMMatrixIdentity());
		m_ScreenFadeEffect.Apply(m_pd3dImmediateContext.Get());
		// 将保存的纹理输出到屏幕
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_FullScreenShow.modelParts[0].vertexBuffer.GetAddressOf(), strides, offsets);
		m_pd3dImmediateContext->IASetIndexBuffer(m_FullScreenShow.modelParts[0].indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		m_pd3dImmediateContext->DrawIndexed(6, 0, 0);
		// 务必解除绑定在着色器上的资源，因为下一帧开始它会作为渲染目标
		m_ScreenFadeEffect.SetTexture(nullptr);
		m_ScreenFadeEffect.Apply(m_pd3dImmediateContext.Get());
	}
	
	// 若截屏键Q按下，则分别保存到output.dds和output.png中
	if (m_PrintScreenStarted)
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		// 输出截屏
		m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
		HR(SaveDDSTextureToFile(m_pd3dImmediateContext.Get(), backBuffer.Get(), L"Screenshot\\output.dds"));
		HR(SaveWICTextureToFile(m_pd3dImmediateContext.Get(), backBuffer.Get(), GUID_ContainerFormatPng, L"Screenshot\\output.png"));
		// 结束截屏
		m_PrintScreenStarted = false;
	}



	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式: 第一人称  Esc退出\n"
			"鼠标移动控制视野 W/S/A/D移动\n"
			"Q-截屏(输出output.dds和output.png到ScreenShot文件夹)";



		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{

	// ******************
	// 初始化用于Render-To-Texture的对象
	//
	m_pMinimapRender = std::make_unique<TextureRender>(m_pd3dDevice.Get(), 400, 400, true);
	m_pScreenFadeRender = std::make_unique<TextureRender>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, false);

	// ******************
	// 初始化游戏对象
	//

	// 创建随机的树
	CreateRandomTrees();

	// 初始化地面
	m_ObjReader.Read(L"Model\\ground.mbo", L"Model\\ground.obj");
	m_Ground.SetModel(Model(m_pd3dDevice.Get(), m_ObjReader));

	// 初始化网格，放置在右下角200x200
	m_Minimap.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow(0.75f, -0.66666666f, 0.25f, 0.33333333f));
	
	// 覆盖整个屏幕面的网格模型
	m_FullScreenShow.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow());

	// ******************
	// 初始化摄像机
	//

	// 默认摄像机
	m_CameraMode = CameraMode::FirstPerson;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();
	

	// 小地图摄像机
	m_MinimapCamera = std::unique_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_MinimapCamera->SetViewPort(0.0f, 0.0f, 200.0f, 200.0f);	// 200x200小地图
	m_MinimapCamera->LookTo(
		XMVectorSet(0.0f, 10.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, -1.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
	m_MinimapCamera->UpdateViewMatrix();

	// ******************
	// 初始化几乎不会变化的值
	//

	// 黑夜特效
	m_BasicEffect.SetFogColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	m_BasicEffect.SetFogStart(5.0f);
	m_BasicEffect.SetFogRange(20.0f);

	// 小地图范围可视
	m_MinimapEffect.SetFogState(true);
	m_MinimapEffect.SetInvisibleColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	m_MinimapEffect.SetMinimapRect(XMVectorSet(-95.0f, 95.0f, 95.0f, -95.0f));
	m_MinimapEffect.SetVisibleRange(25.0f);

	// 方向光(默认)
	DirectionalLight dirLight[4];
	dirLight[0].ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
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
	// 渲染小地图纹理
	// 

	m_BasicEffect.SetViewMatrix(m_MinimapCamera->GetViewXM());
	m_BasicEffect.SetProjMatrix(XMMatrixOrthographicLH(190.0f, 190.0f, 1.0f, 20.0f));	// 使用正交投影矩阵(中心在摄像机位置)
	// 关闭雾效
	m_BasicEffect.SetFogState(false);
	m_pMinimapRender->Begin(m_pd3dImmediateContext.Get());
	DrawScene(true);
	m_pMinimapRender->End(m_pd3dImmediateContext.Get());

	m_MinimapEffect.SetTexture(m_pMinimapRender->GetOutputTexture());


	// 开启雾效，恢复投影矩阵并设置偏暗的光照
	m_BasicEffect.SetFogState(true);
	m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
	dirLight[0].ambient = XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f);
	dirLight[0].diffuse = XMFLOAT4(0.16f, 0.16f, 0.16f, 1.0f);
	for (int i = 0; i < 4; ++i)
		m_BasicEffect.SetDirLight(i, dirLight[i]);
	
	// ******************
	// 设置调试对象名
	//
	m_Trees.SetDebugObjectName("Trees");
	m_Ground.SetDebugObjectName("Ground");
	m_FullScreenShow.SetDebugObjectName("FullScreenShow");
	m_Minimap.SetDebugObjectName("Minimap");
	m_pMinimapRender->SetDebugObjectName("Minimap");
	m_pScreenFadeRender->SetDebugObjectName("ScreenFade");
	

	return true;
}

void GameApp::DrawScene(bool drawMinimap)
{

	m_BasicEffect.SetTextureUsed(true);


	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
	if (drawMinimap)
	{
		// 小地图下绘制所有树
		m_Trees.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, m_InstancedData);
	}
	else
	{
		// 统计实际绘制的物体数目
		std::vector<XMMATRIX> acceptedData;
		// 默认视锥体裁剪
		acceptedData = Collision::FrustumCulling(m_InstancedData, m_Trees.GetLocalBoundingBox(),
			m_pCamera->GetViewXM(), m_pCamera->GetProjXM());
		// 默认硬件实例化绘制
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
		m_Trees.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, acceptedData);
	}
	
	// 绘制地面
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
}

void GameApp::CreateRandomTrees()
{
	srand((unsigned)time(nullptr));
	// 初始化树
	m_ObjReader.Read(L"Model\\tree.mbo", L"Model\\tree.obj");
	m_Trees.SetModel(Model(m_pd3dDevice.Get(), m_ObjReader));
	XMMATRIX S = XMMatrixScaling(0.015f, 0.015f, 0.015f);

	BoundingBox treeBox = m_Trees.GetLocalBoundingBox();
	// 获取树包围盒顶点
	m_TreeBoxData = Collision::CreateBoundingBox(treeBox, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	// 让树木底部紧贴地面位于y = -2的平面
	treeBox.Transform(treeBox, S);
	XMMATRIX T0 = XMMatrixTranslation(0.0f, -(treeBox.Center.y - treeBox.Extents.y + 2.0f), 0.0f);
	// 随机生成144颗随机朝向的树
	float theta = 0.0f;
	for (int i = 0; i < 16; ++i)
	{
		// 取5-95的半径放置随机的树
		for (int j = 0; j < 3; ++j)
		{
			// 距离越远，树木越多
			for (int k = 0; k < 2 * j + 1; ++k)
			{
				float radius = (float)(rand() % 30 + 30 * j + 5);
				float randomRad = rand() % 256 / 256.0f * XM_2PI / 16;
				XMMATRIX T1 = XMMatrixTranslation(radius * cosf(theta + randomRad), 0.0f, radius * sinf(theta + randomRad));
				XMMATRIX R = XMMatrixRotationY(rand() % 256 / 256.0f * XM_2PI);
				XMMATRIX World = S * R * T0 * T1;
				m_InstancedData.push_back(World);
			}
		}
		theta += XM_2PI / 16;
	}

	m_Trees.ResizeBuffer(m_pd3dDevice.Get(), 144);
}
