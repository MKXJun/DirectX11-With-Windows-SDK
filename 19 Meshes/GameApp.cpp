#include "GameApp.h"
#include <filesystem>
#include <algorithm>

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

	if (!mBasicObjectFX.InitAll(md3dDevice))
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
	if (mBasicObjectFX.IsInit())
	{
		mCamera->SetFrustum(XM_PIDIV2, AspectRatio(), 0.5f, 1000.0f);
		mCBChangesOnReSize.proj = mCamera->GetProj();
		mBasicObjectFX.UpdateConstantBuffer(mCBChangesOnReSize);
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
	
	// 第三人称摄像机的操作

	// 绕物体旋转
	cam3rd->RotateX(mouseState.y * dt * 1.25f);
	cam3rd->RotateY(mouseState.x * dt * 1.25f);
	cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
	
	// 更新观察矩阵
	mCamera->UpdateViewMatrix();
	XMStoreFloat4(&mCBFrame.eyePos, mCamera->GetPositionXM());
	mCBFrame.view = mCamera->GetView();

	// 重置滚轮值
	mMouse->ResetScrollWheelValue();

	// 退出程序，这里应向窗口发送销毁信息
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
	
	// 更新每帧变化的值
	mBasicObjectFX.UpdateConstantBuffer(mCBFrame);
}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	mBasicObjectFX.SetRenderDefault();
	
	mGround.Draw(md3dImmediateContext);
	mHouse.Draw(md3dImmediateContext);
	

	//
	// 绘制Direct2D部分
	//
	md2dRenderTarget->BeginDraw();
	std::wstring text = L"当前摄像机模式: 第三人称  Esc退出\n"
		"鼠标移动控制视野 滚轮控制第三人称观察距离\n";
	md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
		D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
	HR(md2dRenderTarget->EndDraw());

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	


	// ******************
	// 初始化游戏对象
	// 初始化地板
	mObjReader.Read(L"Model\\ground.mbo", L"Model\\ground.obj");
	mGround.SetModel(md3dDevice, mObjReader);

	// 初始化房屋模型
	mObjReader.Read(L"Model\\house.mbo", L"Model\\house.obj");
	mHouse.SetModel(md3dDevice, mObjReader);
	BoundingBox mHouseBox;
	// 获取房屋包围盒
	mHouse.GetBoundingBox(mHouseBox, XMMatrixScaling(0.015f, 0.015f, 0.015f));
	// 让房屋底部紧贴地面
	mHouse.SetWorldMatrix(XMMatrixScaling(0.015f, 0.015f, 0.015f) *
		XMMatrixTranslation(0.0f, -(mHouseBox.Center.y - mHouseBox.Extents.y + 1.0f), 0.0f));
	// ******************
	// 初始化常量缓冲区的值
	// 初始化每帧可能会变化的值
	mCameraMode = CameraMode::ThirdPerson;
	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	mCamera = camera;
	
	camera->SetTarget(XMFLOAT3(0.0f, 0.5f, 0.0f));
	camera->SetDistance(10.0f);
	camera->SetDistanceMinMax(6.0f, 100.0f);
	mCBFrame.view = mCamera->GetView();
	XMStoreFloat4(&mCBFrame.eyePos, mCamera->GetPositionXM());

	// 初始化仅在窗口大小变动时修改的值
	mCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
	mCBChangesOnReSize.proj = mCamera->GetProj();

	// 初始化不会变化的值
	// 环境光
	mCBNeverChange.dirLight.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCBNeverChange.dirLight.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mCBNeverChange.dirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCBNeverChange.dirLight.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	// 灯光
	mCBNeverChange.pointLight.Position = XMFLOAT3(0.0f, 20.0f, 0.0f);
	mCBNeverChange.pointLight.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mCBNeverChange.pointLight.Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mCBNeverChange.pointLight.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCBNeverChange.pointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mCBNeverChange.pointLight.Range = 30.0f;	

	// 更新不容易被修改的常量缓冲区资源
	mBasicObjectFX.UpdateConstantBuffer(mCBChangesOnReSize);
	mBasicObjectFX.UpdateConstantBuffer(mCBNeverChange);

	return true;
}


