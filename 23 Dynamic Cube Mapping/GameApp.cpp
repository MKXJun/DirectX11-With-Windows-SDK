#include "GameApp.h"
#include "d3dUtil.h"
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

	if (!mBasicEffect.InitAll(md3dDevice))
		return false;

	if (!mSkyEffect.InitAll(md3dDevice))
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

	// 摄像机变更显示
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

	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(mCamera);

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
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());
	mBasicEffect.SetEyePos(mCamera->GetPositionXM());

	// 选择天空盒
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		mSkyBoxMode = SkyBoxMode::Daylight;
		mBasicEffect.SetTextureCube(mDaylight->GetTextureCube());
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		mSkyBoxMode = SkyBoxMode::Sunset;
		mBasicEffect.SetTextureCube(mSunset->GetTextureCube());
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		mSkyBoxMode = SkyBoxMode::Desert;
		mBasicEffect.SetTextureCube(mDesert->GetTextureCube());
	}

	// 选择球的渲染模式
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D4))
	{
		mSphereMode = SphereMode::None;
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D5))
	{
		mSphereMode = SphereMode::Reflection;
	}
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D6))
	{
		mSphereMode = SphereMode::Refraction;
	}
	
	// 滚轮调整折射率
	mEta += mouseState.scrollWheelValue / 12000.0f;
	if (mEta > 1.0f)
	{
		mEta = 1.0f;
	}
	else if (mEta <= 0.2f)
	{
		mEta = 0.2f;
	}
	mBasicEffect.SetRefractionEta(mEta);
		
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
	// 生成动态天空盒
	//

	// 保留当前绘制的渲染目标视图和深度模板视图
	switch (mSkyBoxMode)
	{
	case SkyBoxMode::Daylight: mDaylight->Cache(md3dImmediateContext, mBasicEffect); break;
	case SkyBoxMode::Sunset: mSunset->Cache(md3dImmediateContext, mBasicEffect); break;
	case SkyBoxMode::Desert: mDesert->Cache(md3dImmediateContext, mBasicEffect); break;
	}

	// 绘制动态天空盒的每个面（以球体为中心）
	for (int i = 0; i < 6; ++i)
	{
		switch (mSkyBoxMode)
		{
		case SkyBoxMode::Daylight: mDaylight->BeginCapture(
			md3dImmediateContext, mBasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		case SkyBoxMode::Sunset: mSunset->BeginCapture(
			md3dImmediateContext, mBasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		case SkyBoxMode::Desert: mDesert->BeginCapture(
			md3dImmediateContext, mBasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i)); break;
		}

		// 不绘制中心球
		DrawScene(false);
	}

	// 恢复之前的绘制设定
	switch (mSkyBoxMode)
	{
	case SkyBoxMode::Daylight: mDaylight->Restore(md3dImmediateContext, mBasicEffect, *mCamera); break;
	case SkyBoxMode::Sunset: mSunset->Restore(md3dImmediateContext, mBasicEffect, *mCamera); break;
	case SkyBoxMode::Desert: mDesert->Restore(md3dImmediateContext, mBasicEffect, *mCamera); break;
	}
	
	// ******************
	// 绘制场景
	//

	// 预先清空
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 绘制中心球
	DrawScene(true);
	

	// ******************
	// 绘制Direct2D部分
	//
	md2dRenderTarget->BeginDraw();
	std::wstring text = L"当前摄像机模式: 自由视角  Esc退出\n"
		"鼠标移动控制视野 滚轮调整折射率 W/S/A/D移动\n"
		"切换天空盒: 1-白天 2-日落 3-沙漠\n"
		"中心球模式: 4-无   5-反射 6-折射\n"
		"当前天空盒: ";

	switch (mSkyBoxMode)
	{
	case SkyBoxMode::Daylight: text += L"白天"; break;
	case SkyBoxMode::Sunset: text += L"日落"; break;
	case SkyBoxMode::Desert: text += L"沙漠"; break;
	}

	text += L" 当前模式: ";
	switch (mSphereMode)
	{
	case SphereMode::None: text += L"无"; break;
	case SphereMode::Reflection: text += L"反射"; break;
	case SphereMode::Refraction: text += L"折射\n折射率: " + std::to_wstring(mEta); break;
	}

	md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
		D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
	HR(md2dRenderTarget->EndDraw());

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// 初始化天空盒相关

	mDaylight = std::make_unique<DynamicSkyRender>(
		md3dDevice, md3dImmediateContext, 
		L"Texture\\daylight.jpg", 
		5000.0f, 256);

	mSunset = std::make_unique<DynamicSkyRender>(
		md3dDevice, md3dImmediateContext,
		std::vector<std::wstring>{
		L"Texture\\sunset_posX.bmp", L"Texture\\sunset_negX.bmp",
		L"Texture\\sunset_posY.bmp", L"Texture\\sunset_negY.bmp", 
		L"Texture\\sunset_posZ.bmp", L"Texture\\sunset_negZ.bmp", },
		5000.0f, 256);

	mDesert = std::make_unique<DynamicSkyRender>(
		md3dDevice, md3dImmediateContext,
		L"Texture\\desertcube1024.dds",
		5000.0f, 256);

	mSkyBoxMode = SkyBoxMode::Daylight;
	mBasicEffect.SetTextureCube(mDaylight->GetDynamicTextureCube());

	// 初始化折射率(空气/镜面)
	mSphereMode = SphereMode::Reflection;
	mEta = 1.0f / 1.51f;

	// ******************
	// 初始化游戏对象
	//
	
	Model model;
	// 球体
	model.SetMesh(md3dDevice, Geometry::CreateSphere(1.0f, 30, 30));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), 
		L"Texture\\stone.dds", 
		nullptr, 
		model.modelParts[0].texA.GetAddressOf()));
	model.modelParts[0].texD = model.modelParts[0].texA;
	mSphere.SetModel(std::move(model));
	// 地面
	model.SetMesh(md3dDevice, 
		Geometry::CreatePlane(XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f); 
	model.modelParts[0].material.Reflect = XMFLOAT4();
	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Texture\\floor.dds",
		nullptr,
		model.modelParts[0].texA.GetAddressOf()));
	model.modelParts[0].texD = model.modelParts[0].texA;
	mGround.SetModel(std::move(model));
	// 柱体
	model.SetMesh(md3dDevice,
		Geometry::CreateCylinder(0.5f, 2.0f));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Texture\\bricks.dds",
		nullptr,
		model.modelParts[0].texA.GetAddressOf()));
	model.modelParts[0].texD = model.modelParts[0].texA;
	mCylinder.SetModel(std::move(model));

	// ******************
	// 初始化摄像机
	//
	mCameraMode = CameraMode::Free;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	// 初始化并更新观察矩阵、投影矩阵(摄像机将被固定)
	camera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(camera->GetViewXM());
	mBasicEffect.SetProjMatrix(camera->GetProjXM());


	// ******************
	// 初始化不会变化的值
	//

	// 方向光
	DirectionalLight dirLight[4];
	dirLight[0].Ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
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

	return true;
}

void GameApp::DrawScene(bool drawCenterSphere)
{
	// 绘制模型
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
	mBasicEffect.SetTextureUsed(true);
	
	// 只有球体才有反射或折射效果
	if (drawCenterSphere)
	{
		switch (mSphereMode)
		{
		case SphereMode::None: 
			mBasicEffect.SetReflectionEnabled(false);
			mBasicEffect.SetRefractionEnabled(false);
			break;
		case SphereMode::Reflection:
			mBasicEffect.SetReflectionEnabled(true);
			mBasicEffect.SetRefractionEnabled(false);
			break;
		case SphereMode::Refraction:
			mBasicEffect.SetReflectionEnabled(false);
			mBasicEffect.SetRefractionEnabled(true);
			break;
		}
		mSphere.Draw(md3dImmediateContext, mBasicEffect);
	}
	
	// 绘制其余物体
	mBasicEffect.SetReflectionEnabled(false);
	mBasicEffect.SetRefractionEnabled(false);
	mGround.Draw(md3dImmediateContext, mBasicEffect);

	// 绘制五个圆柱
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
	// 需要固定位置
	static std::vector<XMMATRIX> cyliderWorlds = {
		XMMatrixTranslation(0.0f, -1.99f, 0.0f),
		XMMatrixTranslation(4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, -4.5f),
		XMMatrixTranslation(4.5f, -1.99f, -4.5f)
	};
	mCylinder.DrawInstanced(md3dImmediateContext, mBasicEffect, cyliderWorlds);
	
	// 绘制五个圆球
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
	mSphere.DrawInstanced(md3dImmediateContext, mBasicEffect, sphereWorlds);

	// 绘制天空盒
	mSkyEffect.SetRenderDefault(md3dImmediateContext);
	switch (mSkyBoxMode)
	{
	case SkyBoxMode::Daylight: mDaylight->Draw(md3dImmediateContext, mSkyEffect, 
		(drawCenterSphere ? *mCamera : mDaylight->GetCamera())); break;
	case SkyBoxMode::Sunset: mSunset->Draw(md3dImmediateContext, mSkyEffect,
		(drawCenterSphere ? *mCamera : mSunset->GetCamera())); break;
	case SkyBoxMode::Desert: mDesert->Draw(md3dImmediateContext, mSkyEffect, 
		(drawCenterSphere ? *mCamera : mDesert->GetCamera())); break;
	}
	
}

