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
	HRESULT hr = md2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, md2dRenderTarget.GetAddressOf());
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
		HR(md2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			mColorBrush.GetAddressOf()));
		HR(mdwriteFactory->CreateTextFormat(L"宋体", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
			mTextFormat.GetAddressOf()));
	}
	else
	{
		// 报告异常问题
		assert(md2dRenderTarget);
	}
	
	// 摄像机变更显示
	if (mCamera != nullptr)
	{
		mCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
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
	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(mCamera);
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(mCamera);
	
	if (mCameraMode == CameraMode::Free)
	{
		// ******************
		// 第一人称/自由摄像机的操作
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
	else if (mCameraMode == CameraMode::ThirdPerson)
	{
		// ******************
		// 第三人称摄像机的操作
		//

		cam3rd->SetTarget(mBoltAnim.GetPosition());

		// 绕物体旋转
		cam3rd->RotateX(mouseState.y * dt * 1.25f);
		cam3rd->RotateY(mouseState.x * dt * 1.25f);
		cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
	}

	// 更新观察矩阵
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());

	// 重置滚轮值
	mMouse->ResetScrollWheelValue();

	// ******************
	// 摄像机模式切换
	//
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1) && mCameraMode != CameraMode::ThirdPerson)
	{
		if (!cam3rd)
		{
			cam3rd.reset(new ThirdPersonCamera);
			cam3rd->SetFrustum(XM_PIDIV2, AspectRatio(), 0.5f, 1000.0f);
			mCamera = cam3rd;
		}
		XMFLOAT3 target = mBoltAnim.GetPosition();
		cam3rd->SetTarget(target);
		cam3rd->SetDistance(8.0f);
		cam3rd->SetDistanceMinMax(3.0f, 20.0f);
		// 初始化时朝物体后方看
		// cam3rd->RotateY(-XM_PIDIV2);

		mCameraMode = CameraMode::ThirdPerson;
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::D2) && mCameraMode != CameraMode::Free)
	{
		if (!cam1st)
		{
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PIDIV2, AspectRatio(), 0.5f, 1000.0f);
			mCamera = cam1st;
		}
		// 从闪电动画上方开始
		XMFLOAT3 pos = mBoltAnim.GetPosition();
		XMFLOAT3 look{ 0.0f, 0.0f, 1.0f };
		XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
		pos.y += 3;
		cam1st->LookTo(pos, look, up);

		mCameraMode = CameraMode::Free;
	}
	
	// 退出程序，这里应向窗口发送销毁信息
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
	
	// 更新闪电动画
	static int currBoltFrame = 0;
	static float frameTime = 0.0f;
	mBoltAnim.SetTexture(mBoltSRVs[currBoltFrame].Get());
	if (frameTime > 1.0f / 60)
	{
		currBoltFrame = (currBoltFrame + 1) % 60;
		frameTime -= 1.0f / 60;
	}
	frameTime += dt;
}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	
	
	// *********************
	// 1. 给镜面反射区域写入值1到模板缓冲区
	// 

	mBasicEffect.SetWriteStencilOnly(md3dImmediateContext, 1);
	mMirror.Draw(md3dImmediateContext, mBasicEffect);

	// ***********************
	// 2. 绘制不透明的反射物体
	//

	// 开启反射绘制
	mBasicEffect.SetReflectionState(true);	// 反射开启
	mBasicEffect.SetRenderDefaultWithStencil(md3dImmediateContext, 1);

	mWalls[2].Draw(md3dImmediateContext, mBasicEffect);
	mWalls[3].Draw(md3dImmediateContext, mBasicEffect);
	mWalls[4].Draw(md3dImmediateContext, mBasicEffect);
	mFloor.Draw(md3dImmediateContext, mBasicEffect);
	
	mWoodCrate.Draw(md3dImmediateContext, mBasicEffect);

	// ***********************
	// 3. 绘制不透明反射物体的阴影
	//

	mWoodCrate.SetMaterial(mShadowMat);
	mBasicEffect.SetShadowState(true);			// 反射开启，阴影开启
	mBasicEffect.SetRenderNoDoubleBlend(md3dImmediateContext, 1);

	mWoodCrate.Draw(md3dImmediateContext, mBasicEffect);

	// 恢复到原来的状态
	mBasicEffect.SetShadowState(false);
	mWoodCrate.SetMaterial(mWoodCrateMat);

	// ***********************
	// 4. 绘制需要混合的反射闪电动画和透明物体
	//

	mBasicEffect.SetDrawBoltAnimNoDepthWriteWithStencil(md3dImmediateContext, 1);
	mBoltAnim.Draw(md3dImmediateContext, mBasicEffect);

	mBasicEffect.SetReflectionState(false);		// 反射关闭

	mBasicEffect.SetRenderAlphaBlendWithStencil(md3dImmediateContext, 1);
	mMirror.Draw(md3dImmediateContext, mBasicEffect);
	
	// ************************
	// 5. 绘制不透明的正常物体
	//
	mBasicEffect.SetRenderDefault(md3dImmediateContext);
	
	for (auto& wall : mWalls)
		wall.Draw(md3dImmediateContext, mBasicEffect);
	mFloor.Draw(md3dImmediateContext, mBasicEffect);
	mWoodCrate.Draw(md3dImmediateContext, mBasicEffect);

	// ************************
	// 6. 绘制不透明正常物体的阴影
	//
	mWoodCrate.SetMaterial(mShadowMat);
	mBasicEffect.SetShadowState(true);			// 反射关闭，阴影开启
	mBasicEffect.SetRenderNoDoubleBlend(md3dImmediateContext, 0);

	mWoodCrate.Draw(md3dImmediateContext, mBasicEffect);

	mBasicEffect.SetShadowState(false);			// 阴影关闭
	mWoodCrate.SetMaterial(mWoodCrateMat);

	// ************************
	// 7. 绘制需要混合的闪电动画
	mBasicEffect.SetDrawBoltAnimNoDepthWrite(md3dImmediateContext);
	mBoltAnim.Draw(md3dImmediateContext, mBasicEffect);

	// ******************
	// 绘制Direct2D部分
	//
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();
		std::wstring text = L"切换摄像机模式: 1-第三人称 2-自由视角\n"
			"W/S/A/D 前进/后退/左平移/右平移 (第三人称无效)  Esc退出\n"
			"鼠标移动控制视野 滚轮控制第三人称观察距离\n"
			"当前模式: ";
		if (mCameraMode == CameraMode::ThirdPerson)
			text += L"第三人称";
		else
			text += L"自由视角";
		md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
		HR(md2dRenderTarget->EndDraw());
	}

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	
	// ******************
	// 初始化游戏对象
	//

	ComPtr<ID3D11ShaderResourceView> texture;
	Material material;
	material.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	material.Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 16.0f);

	mWoodCrateMat = material;
	mShadowMat.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mShadowMat.Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	mShadowMat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

	mBoltSRVs.assign(60, nullptr);
	wchar_t wstr[50];
	// 初始化闪电
	for (int i = 1; i <= 60; ++i)
	{
		wsprintf(wstr, L"Texture\\BoltAnim\\Bolt%03d.bmp", i);
		HR(CreateWICTextureFromFile(md3dDevice.Get(), wstr, nullptr, mBoltSRVs[i - 1].GetAddressOf()));
	}

	mBoltAnim.SetBuffer(md3dDevice, Geometry::CreateCylinderNoCap(4.0f, 4.0f));
	// 抬起高度避免深度缓冲区资源争夺
	mBoltAnim.SetWorldMatrix(XMMatrixTranslation(0.0f, 2.01f, 0.0f));
	mBoltAnim.SetMaterial(material);
	
	// 初始化木盒
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\WoodCrate.dds", nullptr, texture.GetAddressOf()));
	mWoodCrate.SetBuffer(md3dDevice, Geometry::CreateBox());
	// 抬起高度避免深度缓冲区资源争夺
	mWoodCrate.SetWorldMatrix(XMMatrixTranslation(0.0f, 0.01f, 0.0f));
	mWoodCrate.SetTexture(texture);
	mWoodCrate.SetMaterial(material);
	

	// 初始化地板
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\floor.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	mFloor.SetBuffer(md3dDevice, 
		Geometry::CreatePlane(XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
	mFloor.SetTexture(texture);
	mFloor.SetMaterial(material);

	// 初始化墙体
	mWalls.resize(5);
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\brick.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	// 这里控制墙体五个面的生成，0和1的中间位置用于放置镜面
	//     ____     ____
	//    /| 0 |   | 1 |\
	//   /4|___|___|___|2\
	//  /_/_ _ _ _ _ _ _\_\
	// | /       3       \ |
	// |/_________________\|
	//
	for (int i = 0; i < 5; ++i)
	{
		mWalls[i].SetMaterial(material);
		mWalls[i].SetTexture(texture);
	}
	mWalls[0].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
	mWalls[1].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
	mWalls[2].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	mWalls[3].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	mWalls[4].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	
	mWalls[0].SetWorldMatrix(XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(-7.0f, 3.0f, 10.0f));
	mWalls[1].SetWorldMatrix(XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(7.0f, 3.0f, 10.0f));
	mWalls[2].SetWorldMatrix(XMMatrixRotationY(-XM_PIDIV2) * XMMatrixRotationZ(XM_PIDIV2) * XMMatrixTranslation(10.0f, 3.0f, 0.0f));
	mWalls[3].SetWorldMatrix(XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(0.0f, 3.0f, -10.0f));
	mWalls[4].SetWorldMatrix(XMMatrixRotationY(XM_PIDIV2) * XMMatrixRotationZ(-XM_PIDIV2) * XMMatrixTranslation(-10.0f, 3.0f, 0.0f));

	// 初始化镜面
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\ice.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	mMirror.SetBuffer(md3dDevice,
		Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(8.0f, 8.0f), XMFLOAT2(1.0f, 1.0f)));
	mMirror.SetWorldMatrix(XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(0.0f, 3.0f, 10.0f));
	mMirror.SetTexture(texture);
	mMirror.SetMaterial(material);

	// ******************
	// 初始化摄像机
	//
	mCameraMode = CameraMode::ThirdPerson;
	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetTarget(XMFLOAT3(0.0f, 0.5f, 0.0f));
	camera->SetDistance(5.0f);
	camera->SetDistanceMinMax(2.0f, 14.0f);
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());
	mBasicEffect.SetEyePos(mCamera->GetPositionXM());

	mCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
	mBasicEffect.SetProjMatrix(mCamera->GetProjXM());

	// ******************
	// 初始化不会变化的值
	//
	mBasicEffect.SetReflectionMatrix(XMMatrixReflect(XMVectorSet(0.0f, 0.0f, -1.0f, 10.0f)));
	// 稍微高一点位置以显示阴影
	mBasicEffect.SetShadowMatrix(XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.99f), XMVectorSet(0.0f, 10.0f, -10.0f, 1.0f)));
	mBasicEffect.SetRefShadowMatrix(XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.99f), XMVectorSet(0.0f, 10.0f, 30.0f, 1.0f)));
	// 环境光
	DirectionalLight dirLight;
	dirLight.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	mBasicEffect.SetDirLight(0, dirLight);
	// 灯光
	PointLight pointLight;
	pointLight.Position = XMFLOAT3(0.0f, 10.0f, -10.0f);
	pointLight.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	pointLight.Diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	pointLight.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	pointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	pointLight.Range = 25.0f;
	mBasicEffect.SetPointLight(0, pointLight);
	

	return true;
}



