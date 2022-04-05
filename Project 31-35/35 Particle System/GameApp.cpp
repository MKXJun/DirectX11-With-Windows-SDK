#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

#pragma warning(disable: 26812)

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance), 
	m_pBasicEffect(std::make_unique<BasicEffect>()),
	m_pFireEffect(std::make_unique<ParticleEffect>()),
	m_pRainEffect(std::make_unique<ParticleEffect>()),
	m_pSkyEffect(std::make_unique<SkyEffect>()),
	m_pFire(std::make_unique<ParticleRender>()),
	m_pRain(std::make_unique<ParticleRender>())
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

	if (!m_pFireEffect->Init(m_pd3dDevice.Get(), L"HLSL\\Fire"))
		return false;

	if (!m_pRainEffect->Init(m_pd3dDevice.Get(), L"HLSL\\Rain"))
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
	}
}

void GameApp::UpdateScene(float dt)
{

	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	// ******************
	// 第一人称摄像机的操作
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

	cam1st->Walk(d1 * 6.0f);
	cam1st->Strafe(d2 * 6.0f);

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		cam1st->Pitch(io.MouseDelta.y * 0.01f);
		cam1st->RotateY(io.MouseDelta.x * 0.01f);
	}

	if (ImGui::Begin("Particle System"))
	{
		if (ImGui::Button("Reset Particle"))
		{
			m_pFire->Reset();
			m_pRain->Reset();
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
	// 第一人称摄像机的操作
	//

	// 方向移动
	if (keyState.IsKeyDown(Keyboard::W))
		cam1st->Walk(dt * 6.0f);
	if (keyState.IsKeyDown(Keyboard::S))
		cam1st->Walk(dt * -6.0f);
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

	if (m_KeyboardTracker.IsKeyPressed(Keyboard::R))
	{
		m_pFire->Reset();
		m_pRain->Reset();
	}

	// 退出程序，这里应向窗口发送销毁信息
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
#endif
	// 将位置限制在[-80.0f, 80.0f]的区域内
	// 不允许穿地
	XMFLOAT3 adjustedPos;
	XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorReplicate(-80.0f), XMVectorReplicate(80.0f)));
	cam1st->SetPosition(adjustedPos);

	m_pBasicEffect->SetViewMatrix(m_pCamera->GetViewXM());
	m_pBasicEffect->SetEyePos(m_pCamera->GetPosition());

	// ******************
	// 粒子系统
	//
	m_pFire->Update(dt, m_Timer.TotalTime());
	m_pRain->Update(dt, m_Timer.TotalTime());

	m_pFireEffect->SetViewProjMatrix(m_pCamera->GetViewProjXM());
	m_pFireEffect->SetEyePos(m_pCamera->GetPosition());

	static XMFLOAT3 lastCameraPos = m_pCamera->GetPosition();
	XMFLOAT3 cameraPos = m_pCamera->GetPosition();

	XMVECTOR cameraPosVec = XMLoadFloat3(&cameraPos);
	XMVECTOR lastCameraPosVec = XMLoadFloat3(&lastCameraPos);
	XMFLOAT3 emitPos;
	XMStoreFloat3(&emitPos, cameraPosVec + 3.0f * (cameraPosVec - lastCameraPosVec));
	m_pRainEffect->SetViewProjMatrix(m_pCamera->GetViewProjXM());
	m_pRainEffect->SetEyePos(m_pCamera->GetPosition());
	m_pRain->SetEmitPos(emitPos);
	lastCameraPos = m_pCamera->GetPosition();
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ******************
	// 正常绘制场景
	//
	
	// 统计实际绘制的物体数目
	std::vector<Transform> acceptedData;
	// 默认视锥体裁剪
	acceptedData = Collision::FrustumCulling(m_InstancedData, m_Trees.GetLocalBoundingBox(),
		m_pCamera->GetViewXM(), m_pCamera->GetProjXM());
	// 默认硬件实例化绘制
	m_pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
	m_Trees.DrawInstanced(m_pd3dImmediateContext.Get(), m_pBasicEffect.get(), acceptedData);

	// 绘制地面
	m_pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	m_Ground.Draw(m_pd3dImmediateContext.Get(), m_pBasicEffect.get());

	// 绘制天空盒
	m_pSkyEffect->SetRenderDefault(m_pd3dImmediateContext.Get());
	m_pGrassCube->Draw(m_pd3dImmediateContext.Get(), *m_pSkyEffect, *m_pCamera);

	// ******************
	// 粒子系统留在最后绘制
	//

	m_pFire->Draw(m_pd3dImmediateContext.Get(), *m_pFireEffect, *m_pCamera);
	m_pRain->Draw(m_pd3dImmediateContext.Get(), *m_pRainEffect, *m_pCamera);



#ifdef USE_IMGUI
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#else
	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式: 第一人称  Esc退出\nR-重置粒子系统";
		
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
	// 初始化摄像机
	//

	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;

	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(XMFLOAT3(0.0f, 0.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

	// ******************
	// 初始化特效
	//

	m_pBasicEffect->SetTextureUsed(true);
	m_pBasicEffect->SetShadowEnabled(false);
	m_pBasicEffect->SetSSAOEnabled(false);
	m_pBasicEffect->SetViewMatrix(camera->GetViewXM());
	m_pBasicEffect->SetProjMatrix(camera->GetProjXM());

	m_pFireEffect->SetBlendState(RenderStates::BSAlphaWeightedAdditive.Get(), nullptr, 0xFFFFFFFF);
	m_pFireEffect->SetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);

	m_pRainEffect->SetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);

	// ******************
	// 初始化对象
	//

	// 创建随机的树
	CreateRandomTrees();

	// 初始化地面
	m_ObjReader.Read(L"..\\Model\\ground_35.mbo", L"..\\Model\\ground_35.obj");
	m_Ground.SetModel(Model(m_pd3dDevice.Get(), m_ObjReader));

	// ******************
	// 初始化粒子系统
	//
	ComPtr<ID3D11ShaderResourceView> pFlareSRV, pRainSRV, pRandomSRV;
	HR(CreateTexture2DArrayFromFile(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		std::vector<std::wstring>{ L"..\\Texture\\flare0.dds" }, nullptr, pFlareSRV.GetAddressOf()));
	HR(CreateRandomTexture1D(m_pd3dDevice.Get(), nullptr, pRandomSRV.GetAddressOf()));
	m_pFire->Init(m_pd3dDevice.Get(), 500);
	m_pFire->SetTextureArraySRV(pFlareSRV.Get());
	m_pFire->SetRandomTexSRV(pRandomSRV.Get());
	m_pFire->SetEmitPos(XMFLOAT3(0.0f, -1.0f, 0.0f));
	m_pFire->SetEmitDir(XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_pFire->SetEmitInterval(0.005f);
	m_pFire->SetAliveTime(1.0f);
	

	HR(CreateTexture2DArrayFromFile(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		std::vector<std::wstring>{ L"..\\Texture\\raindrop.dds" }, nullptr, pRainSRV.GetAddressOf()));
	HR(CreateRandomTexture1D(m_pd3dDevice.Get(), nullptr, pRandomSRV.ReleaseAndGetAddressOf()));
	m_pRain->Init(m_pd3dDevice.Get(), 10000);
	m_pRain->SetTextureArraySRV(pRainSRV.Get());
	m_pRain->SetRandomTexSRV(pRandomSRV.Get());
	m_pRain->SetEmitDir(XMFLOAT3(0.0f, -1.0f, 0.0f));
	m_pRain->SetEmitInterval(0.0015f);
	m_pRain->SetAliveTime(3.0f);

	// ******************
	// 初始化天空盒相关
	//
	m_pGrassCube = std::make_unique<SkyRender>();
	HR(m_pGrassCube->InitResource(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		L"..\\Texture\\grasscube1024.dds"));

	m_pBasicEffect->SetTextureCube(m_pGrassCube->GetTextureCube());

	// ******************
	// 初始化光照
	//
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
		m_pBasicEffect->SetDirLight(i, dirLight[i]);
		


	
	// ******************
	// 设置调试对象名
	//
	m_Ground.SetDebugObjectName("Ground");
	m_pGrassCube->SetDebugObjectName("GrassCube");
	m_pFireEffect->SetDebugObjectName("FireEffect");
	m_pRainEffect->SetDebugObjectName("RainEffect");
	m_pFire->SetDebugObjectName("Fire");
	m_pRain->SetDebugObjectName("Rain");

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
	// 随机生成144颗随机朝向的树
	m_InstancedData.resize(144);
	m_Trees.ResizeBuffer(m_pd3dDevice.Get(), 144);
	float theta = 0.0f;
	int pos = 0;
	for (int i = 0; i < 16; ++i)
	{
		// 取5-95的半径放置随机的树
		for (int j = 0; j < 3; ++j)
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
