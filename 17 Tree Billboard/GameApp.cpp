#include "GameApp.h"
#include "d3dUtil.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
	// 开启4倍多重采样
	mEnable4xMsaa = true;
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(md3dDevice);

	if (!mBasicEffect.InitAll(md3dDevice))
		return false;

	if (!InitResource())
		return false;

	// 初始化鼠标，键盘不需要
	mMouse->SetWindow(mhMainWnd);
	mMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

	return true;
}

void GameApp::OnResize()
{
	assert(md2dFactory);
	assert(mdwriteFactory);
	// 释放D2D的相关资源
	mColorBrush.Reset();
	md2dRenderTarget.Reset();

	D3DApp::OnResize();

	// 为D2D创建DXGI表面渲染目标
	ComPtr<IDXGISurface> surface;
	HR(mSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HR(md2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, md2dRenderTarget.GetAddressOf()));

	surface.Reset();
	// 创建固定颜色刷和文本格式
	HR(md2dRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::White),
		mColorBrush.GetAddressOf()));
	HR(mdwriteFactory->CreateTextFormat(L"宋体", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
		mTextFormat.GetAddressOf()));

	if (mCamera != nullptr)
	{
		mCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		mCamera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
		mBasicEffect.SetProjMatrix(mCamera->GetProjXM());
	}

}

void GameApp::UpdateScene(float dt)
{

	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = mMouse->GetState();
	Mouse::State lastMouseState = mMouseTracker.GetLastState();
	mMouseTracker.Update(mouseState);

	Keyboard::State keyState = mKeyboard->GetState();
	mKeyboardTracker.Update(keyState);

	// 获取子类
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(mCamera);

	if (mCameraMode == CameraMode::Free)
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
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetEyePos(mCamera->GetPositionXM());
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());

	// ******************
	// 开关雾效
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		mFogEnabled = !mFogEnabled;
		mBasicEffect.SetFogState(mFogEnabled);
	}

	// ******************
	// 白天/黑夜变换
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		mIsNight = !mIsNight;
		if (mIsNight)
		{
			// 黑夜模式下变为逐渐黑暗
			mBasicEffect.SetFogColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
			mBasicEffect.SetFogStart(5.0f);
		}
		else
		{
			// 白天模式则对应雾效
			mBasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
			mBasicEffect.SetFogStart(15.0f);
		}
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		mEnableAlphaToCoverage = !mEnableAlphaToCoverage;
	}

	// ******************
	// 调整雾的范围
	//
	if (mouseState.scrollWheelValue != 0)
	{
		// 一次滚轮滚动的最小单位为120
		mFogRange += mouseState.scrollWheelValue / 120;
		if (mFogRange < 15.0f)
			mFogRange = 15.0f;
		else if (mFogRange > 175.0f)
			mFogRange = 175.0f;
		mBasicEffect.SetFogRange(mFogRange);
	}
	

	// 重置滚轮值
	mMouse->ResetScrollWheelValue();

	

	// 退出程序，这里应向窗口发送销毁信息
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);

}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	// ******************
	// 绘制Direct3D部分
	//
	
	// 设置背景色
	if (mIsNight)
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	}
	else
	{
		md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	}
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 绘制地面
	mBasicEffect.SetRenderDefault(md3dImmediateContext);
	mGround.Draw(md3dImmediateContext, mBasicEffect);

	// 绘制树
	mBasicEffect.SetRenderBillboard(md3dImmediateContext, mEnableAlphaToCoverage);
	mBasicEffect.SetMaterial(mTreeMat);
	UINT stride = sizeof(VertexPosSize);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, mPointSpritesBuffer.GetAddressOf(), &stride, &offset);
	mBasicEffect.Apply(md3dImmediateContext);
	md3dImmediateContext->Draw(16, 0);

	// ******************
	// 绘制Direct2D部分
	//
	md2dRenderTarget->BeginDraw();
	std::wstring text = L"1-雾效开关 2-白天/黑夜雾效切换 3-AlphaToCoverage开关 Esc-退出\n"
		"滚轮-调整雾效范围\n"
		"仅支持自由视角摄像机\n";
	text += std::wstring(L"AlphaToCoverage状态: ") + (mEnableAlphaToCoverage ? L"开启\n" : L"关闭\n");
	text += std::wstring(L"雾效状态: ") + (mFogEnabled ? L"开启\n" : L"关闭\n");
	if (mFogEnabled)
	{
		text += std::wstring(L"天气情况: ") + (mIsNight ? L"黑夜\n" : L"白天\n");
		text += L"雾效范围: " + std::to_wstring(mIsNight ? 5 : 15) + L"-" + 
			std::to_wstring((mIsNight ? 5 : 15) + (int)mFogRange);
	}


	md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
		D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
	HR(md2dRenderTarget->EndDraw());

	HR(mSwapChain->Present(0, 0));

}



bool GameApp::InitResource()
{
	// ******************
	// 初始化各种物体
	//

	// 初始化树纹理资源
	mTreeTexArray = CreateDDSTexture2DArrayFromFile(
		md3dDevice,
		md3dImmediateContext,
		std::vector<std::wstring>{
			L"Texture\\tree0.dds",
			L"Texture\\tree1.dds",
			L"Texture\\tree2.dds",
			L"Texture\\tree3.dds"});
	mBasicEffect.SetTextureArray(mTreeTexArray);

	// 初始化点精灵缓冲区
	InitPointSpritesBuffer();

	// 初始化树的材质
	mTreeMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mTreeMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mTreeMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	ComPtr<ID3D11ShaderResourceView> texture;
	// 初始化地板
	mGround.SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, -5.0f, 0.0f), XMFLOAT2(100.0f, 100.0f), XMFLOAT2(10.0f, 10.0f)));
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\Grass.dds", nullptr, texture.GetAddressOf()));
	mGround.SetTexture(texture);
	Material material;
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	mGround.SetMaterial(material);

	// ******************
	// 初始化不会变化的值
	//

	// 方向光
	DirectionalLight dirLight[4];
	dirLight[0].Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].Diffuse = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	dirLight[0].Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].Direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	dirLight[1] = dirLight[0];
	dirLight[1].Direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
	dirLight[2] = dirLight[0];
	dirLight[2].Direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
	dirLight[3] = dirLight[0];
	dirLight[3].Direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
	for (int i = 0; i < 4; ++i)
		mBasicEffect.SetDirLight(i, dirLight[i]);

	// ******************
	// 初始化摄像机
	//
	mCameraMode = CameraMode::Free;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();

	mBasicEffect.SetWorldViewProjMatrix(XMMatrixIdentity(), camera->GetViewXM(), camera->GetProjXM());
	mBasicEffect.SetEyePos(camera->GetPositionXM());

	// ******************
	// 初始化雾效和天气等
	//

	// 默认白天，开启AlphaToCoverage
	mIsNight = false;
	mEnableAlphaToCoverage = true;

	// 雾状态默认开启
	mFogEnabled = 1;
	mFogRange = 75.0f;

	mBasicEffect.SetFogState(mFogEnabled);
	mBasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
	mBasicEffect.SetFogStart(15.0f);
	mBasicEffect.SetFogRange(75.0f);

	
	
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
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mPointSpritesBuffer.GetAddressOf()));
}
