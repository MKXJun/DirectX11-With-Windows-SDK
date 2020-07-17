#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_CameraMode(CameraMode::Free),
	m_EnableFrustumCulling(true),
	m_EnableInstancing(true)
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
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);
	}

	// ******************
	// 更新摄像机相关
	//

	// 将位置限制在[-119.9f, 119.9f]的区域内
	// 不允许穿地
	XMFLOAT3 adjustedPos;
	XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(),
		XMVectorSet(-119.9f, 0.0f, -119.9f, 0.0f), XMVectorSet(119.9f, 99.9f, 119.9f, 0.0f)));
	cam1st->SetPosition(adjustedPos);

	m_BasicEffect.SetEyePos(m_pCamera->GetPosition());
	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());

	// ******************
	// 视锥体裁剪开关
	//
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		m_EnableInstancing = !m_EnableInstancing;
	}
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_EnableFrustumCulling = !m_EnableFrustumCulling;
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
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 统计实际绘制的物体数目
	std::vector<Transform> acceptedData;
	// 是否开启视锥体裁剪
	if (m_EnableFrustumCulling)
	{
		acceptedData = Collision::FrustumCulling(m_InstancedData, m_Trees.GetLocalBoundingBox(),
			m_pCamera->GetViewXM(), m_pCamera->GetProjXM());
	}
	// 确定使用的数据集
	const std::vector<Transform>& refData = m_EnableFrustumCulling ? acceptedData : m_InstancedData;
	// 是否开启硬件实例化
	if (m_EnableInstancing)
	{
		// 硬件实例化绘制
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
		m_Trees.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, refData);
	}
	else
	{
		// 遍历的形式逐个绘制
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
		for (const Transform& t : refData)
		{
			m_Trees.GetTransform() = t;
			m_Trees.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
		}
	}

	// 绘制地面
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式: 自由视角  Esc退出\n"
			L"鼠标移动控制视野 WSAD移动\n"
			L"数字键1:硬件实例化开关 2:视锥体裁剪开关\n"
			L"硬件实例化: ";
		text += (m_EnableInstancing ? L"开" : L"关");
		text += L" 视锥体裁剪: ";
		text += (m_EnableFrustumCulling ? L"开" : L"关");
		text += L"\n绘制树木数: " + std::to_wstring(refData.size());
		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// 初始化游戏对象
	//

	// 创建随机的树
	CreateRandomTrees();

	// 初始化地面
	m_ObjReader.Read(L"..\\Model\\ground_20.mbo", L"..\\Model\\ground_20.obj");
	m_Ground.SetModel(Model(m_pd3dDevice.Get(), m_ObjReader));

	// ******************
	// 初始化摄像机
	//
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(XMFLOAT3(), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_BasicEffect.SetViewMatrix(camera->GetViewXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjXM());

	// ******************
	// 初始化不会变化的值
	//

	// 方向光
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
	// 设置调试对象名
	//
	m_Ground.SetDebugObjectName("Ground");
	m_Trees.SetDebugObjectName("Trees");


	return true;
}

void GameApp::CreateRandomTrees()
{
	srand((unsigned)time(nullptr));
	// 初始化树
	m_ObjReader.Read(L"..\\Model\\tree.mbo", L"..\\Model\\tree.obj");
	m_Trees.SetModel(Model(m_pd3dDevice.Get(), m_ObjReader));
	XMMATRIX S = XMMatrixScaling(0.015f, 0.015f, 0.015f);
	
	BoundingBox treeBox = m_Trees.GetLocalBoundingBox();

	// 让树木底部紧贴地面位于y = -2的平面
	treeBox.Transform(treeBox, S);
	float Ty = -(treeBox.Center.y - treeBox.Extents.y + 2.0f);
	// 随机生成256颗随机朝向的树
	m_InstancedData.resize(256);
	m_Trees.ResizeBuffer(m_pd3dDevice.Get(), 256);

	float theta = 0.0f;
	int pos = 0;
	for (int i = 0; i < 16; ++i)
	{
		// 取5-125的半径放置随机的树
		for (int j = 0; j < 4; ++j)
		{
			// 距离越远，树木越多
			for (int k = 0; k < 2 * j + 1; ++k, ++pos)
			{
				float radius = (float)(rand() % 30 + 30 * j + 5);
				float randomRad = rand() % 256 / 256.0f * XM_2PI / 16;
				m_InstancedData[pos].SetScale(0.015f, 0.015f, 0.015f);
				m_InstancedData[pos].SetRotation(0.0f, rand() % 256 / 256.0f * XM_2PI, 0.0f);
				m_InstancedData[pos].SetPosition(radius * cosf(theta + randomRad), Ty, radius * sinf(theta + randomRad));
			}
		}
		theta += XM_2PI / 16;
	}

	
}

