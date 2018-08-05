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

	if (!mBasicFX.InitAll(md3dDevice))
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
	if (mBasicFX.IsInit())
	{
		mCamera->SetFrustum(XM_PIDIV2, AspectRatio(), 0.5f, 1000.0f);
		mCBOnReSize.proj = mCamera->GetProj();
		mBasicFX.UpdateConstantBuffer(mCBOnReSize);
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
	
	if (mCameraMode == CameraMode::FirstPerson || mCameraMode == CameraMode::Free)
	{
		// 第一人称/自由摄像机的操作

		// 方向移动
		if (keyState.IsKeyDown(Keyboard::W))
		{
			if (mCameraMode == CameraMode::FirstPerson)
				cam1st->Walk(dt * 3.0f);
			else
				cam1st->MoveForward(dt * 3.0f);
		}
		if (keyState.IsKeyDown(Keyboard::S))
		{
			if (mCameraMode == CameraMode::FirstPerson)
				cam1st->Walk(dt * -3.0f);
			else
				cam1st->MoveForward(dt * -3.0f);
		}
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		// 将位置限制在[-8.9f, 8.9f]的区域内
		// 不允许穿地
		XMFLOAT3 adjustedPos;
		XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorSet(-8.9f, 0.0f, -8.9f, 0.0f), XMVectorReplicate(8.9f)));
		cam1st->SetPosition(adjustedPos);

		// 仅在第一人称模式移动箱子
		if (mCameraMode == CameraMode::FirstPerson)
			mWoodCrate.SetWorldMatrix(XMMatrixTranslation(adjustedPos.x, adjustedPos.y, adjustedPos.z));
		// 视野旋转，防止开始的差值过大导致的突然旋转
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);
	}
	else if (mCameraMode == CameraMode::ThirdPerson)
	{
		// 第三人称摄像机的操作

		cam3rd->SetTarget(mWoodCrate.GetPosition());

		// 绕物体旋转
		cam3rd->RotateX(mouseState.y * dt * 1.25f);
		cam3rd->RotateY(mouseState.x * dt * 1.25f);
		cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
	}

	// 更新观察矩阵
	mCamera->UpdateViewMatrix();
	XMStoreFloat4(&mCBFrame.eyePos, mCamera->GetPositionXM());
	mCBFrame.view = mCamera->GetView();

	// 重置滚轮值
	mMouse->ResetScrollWheelValue();

	// 摄像机模式切换
	if (keyState.IsKeyDown(Keyboard::D1) && mCameraMode != CameraMode::FirstPerson)
	{
		if (!cam1st)
		{
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PIDIV2, AspectRatio(), 0.5f, 1000.0f);
			mCamera = cam1st;
		}
		XMFLOAT3 pos = mWoodCrate.GetPosition();
		XMFLOAT3 target = (!pos.x && !pos.z ? XMFLOAT3{ 0.0f, 0.0f, 1.0f } : XMFLOAT3{});
		cam1st->LookAt(pos, target, XMFLOAT3(0.0f, 1.0f, 0.0f));

		mCameraMode = CameraMode::FirstPerson;
	}
	else if (keyState.IsKeyDown(Keyboard::D2) && mCameraMode != CameraMode::ThirdPerson)
	{
		if (!cam3rd)
		{
			cam3rd.reset(new ThirdPersonCamera);
			cam3rd->SetFrustum(XM_PIDIV2, AspectRatio(), 0.5f, 1000.0f);
			mCamera = cam3rd;
		}
		XMFLOAT3 target = mWoodCrate.GetPosition();
		cam3rd->SetTarget(target);
		cam3rd->SetDistance(8.0f);
		cam3rd->SetDistanceMinMax(3.0f, 20.0f);
		// 初始化时朝物体后方看
		// cam3rd->RotateY(-XM_PIDIV2);

		mCameraMode = CameraMode::ThirdPerson;
	}
	else if (keyState.IsKeyDown(Keyboard::D3) && mCameraMode != CameraMode::Free)
	{
		if (!cam1st)
		{
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PIDIV2, AspectRatio(), 0.5f, 1000.0f);
			mCamera = cam1st;
		}
		// 从箱子上方开始
		XMFLOAT3 pos = mWoodCrate.GetPosition();
		XMFLOAT3 look{ 0.0f, 0.0f, 1.0f };
		XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
		pos.y += 3;
		cam1st->LookTo(pos, look, up);

		mCameraMode = CameraMode::Free;
	}
	
	// 退出程序，这里应向窗口发送销毁信息
	if (keyState.IsKeyDown(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
	
	mBasicFX.UpdateConstantBuffer(mCBFrame);
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

	mBasicFX.SetWriteStencilOnly(1);
	mMirror.Draw(md3dImmediateContext);

	// ***********************
	// 2. 绘制不透明的反射物体
	//

	// 开启反射绘制
	mDrawingState.isReflection = 1;		// 反射开启
	mBasicFX.UpdateConstantBuffer(mDrawingState);
	mBasicFX.SetRenderDefaultWithStencil(1);

	mWalls[2].Draw(md3dImmediateContext);
	mWalls[3].Draw(md3dImmediateContext);
	mWalls[4].Draw(md3dImmediateContext);
	mFloor.Draw(md3dImmediateContext);
	mWoodCrate.Draw(md3dImmediateContext);

	// 绘制阴影
	mWoodCrate.SetMaterial(mShadowMat);
	mDrawingState.isShadow = 1;			// 反射开启，阴影开启
	mBasicFX.UpdateConstantBuffer(mDrawingState);
	mBasicFX.SetRenderNoDoubleBlend(1);

	mWoodCrate.Draw(md3dImmediateContext);

	// 恢复到原来的状态
	mDrawingState.isShadow = 0;
	mBasicFX.UpdateConstantBuffer(mDrawingState);
	mWoodCrate.SetMaterial(mWoodCrateMat);
	

	// ***********************
	// 3. 绘制透明镜面
	//

	mBasicFX.SetRenderAlphaBlendWithStencil(1);

	mMirror.Draw(md3dImmediateContext);
	
	// 关闭反射绘制
	mDrawingState.isReflection = 0;
	mBasicFX.UpdateConstantBuffer(mDrawingState);
	

	// ************************
	// 4. 绘制不透明的正常物体
	//
	mBasicFX.SetRenderDefault();

	for (auto& wall : mWalls)
		wall.Draw(md3dImmediateContext);
	mFloor.Draw(md3dImmediateContext);
	mWoodCrate.Draw(md3dImmediateContext);

	// 绘制阴影
	mWoodCrate.SetMaterial(mShadowMat);
	mDrawingState.isShadow = 1;			// 反射关闭，阴影开启
	mBasicFX.UpdateConstantBuffer(mDrawingState);
	mBasicFX.SetRenderNoDoubleBlend(0);

	mWoodCrate.Draw(md3dImmediateContext);

	mDrawingState.isShadow = 0;			// 阴影关闭
	mBasicFX.UpdateConstantBuffer(mDrawingState);
	mWoodCrate.SetMaterial(mWoodCrateMat);


	//
	// 绘制Direct2D部分
	//
	md2dRenderTarget->BeginDraw();
	std::wstring text = L"切换摄像机模式: 1-第一人称 2-第三人称 3-自由视角\n"
		"W/S/A/D 前进/后退/左平移/右平移 (第三人称无效)  Esc退出\n"
		"鼠标移动控制视野 滚轮控制第三人称观察距离\n"
		"当前模式: ";
	if (mCameraMode == CameraMode::FirstPerson)
		text += L"第一人称(控制箱子移动)";
	else if (mCameraMode == CameraMode::ThirdPerson)
		text += L"第三人称";
	else
		text += L"自由视角";
	md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
		D2D1_RECT_F{ 0.0f, 0.0f, 500.0f, 60.0f }, mColorBrush.Get());
	HR(md2dRenderTarget->EndDraw());

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	
	// ******************
	// 初始化游戏对象
	ComPtr<ID3D11ShaderResourceView> texture;
	Material material;
	material.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	material.Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 16.0f);

	mWoodCrateMat = material;
	mShadowMat.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mShadowMat.Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	mShadowMat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

	// 初始化木盒
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\WoodCrate.dds", nullptr, texture.GetAddressOf()));
	mWoodCrate.SetBuffer(md3dDevice, Geometry::CreateBox());
	// 抬起高度避免深度缓冲区资源争夺
	mWoodCrate.SetWorldMatrix(XMMatrixTranslation(0.0f, 0.01f, 5.0f));
	mWoodCrate.SetTexTransformMatrix(XMMatrixIdentity());
	mWoodCrate.SetTexture(texture);
	mWoodCrate.SetMaterial(material);
	
	

	// 初始化地板
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\floor.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	mFloor.SetBuffer(md3dDevice, 
		Geometry::CreatePlane(XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
	mFloor.SetWorldMatrix(XMMatrixIdentity());
	mFloor.SetTexTransformMatrix(XMMatrixIdentity());
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
		mWalls[i].SetTexTransformMatrix(XMMatrixIdentity());
		mWalls[i].SetTexture(texture);
	}
	mWalls[0].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(), XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
	mWalls[1].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(), XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
	mWalls[2].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	mWalls[3].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	mWalls[4].SetBuffer(md3dDevice, Geometry::CreatePlane(XMFLOAT3(), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	
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
		Geometry::CreatePlane(XMFLOAT3(), XMFLOAT2(8.0f, 8.0f), XMFLOAT2(1.0f, 1.0f)));
	mMirror.SetWorldMatrix(XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(0.0f, 3.0f, 10.0f));
	mMirror.SetTexTransformMatrix(XMMatrixIdentity());
	mMirror.SetTexture(texture);
	mMirror.SetMaterial(material);

	// ******************
	// 初始化常量缓冲区的值
	// 初始化每帧可能会变化的值
	mCameraMode = CameraMode::ThirdPerson;
	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	mCamera = camera;
	
	camera->SetTarget(XMFLOAT3(0.0f, 0.5f, 0.0f));
	camera->SetDistance(5.0f);
	camera->SetDistanceMinMax(2.0f, 14.0f);
	mCBFrame.view = mCamera->GetView();
	XMStoreFloat4(&mCBFrame.eyePos, mCamera->GetPositionXM());

	// 初始化仅在窗口大小变动时修改的值
	mCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
	mCBOnReSize.proj = mCamera->GetProj();

	// 初始化不会变化的值
	mCBNeverChange.reflection = XMMatrixReflect(XMVectorSet(0.0f, 0.0f, -1.0f, 10.0f));
	// 稍微高一点位置以显示阴影
	mCBNeverChange.shadow = XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.99f), XMVectorSet(0.0f, 10.0f, -10.0f, 1.0f));
	mCBNeverChange.refShadow = XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.99f), XMVectorSet(0.0f, 10.0f, 30.0f, 1.0f));
	// 环境光
	mCBNeverChange.dirLight[0].Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCBNeverChange.dirLight[0].Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mCBNeverChange.dirLight[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCBNeverChange.dirLight[0].Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	// 灯光
	mCBNeverChange.pointLight[0].Position = XMFLOAT3(0.0f, 10.0f, -10.0f);
	mCBNeverChange.pointLight[0].Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mCBNeverChange.pointLight[0].Diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	mCBNeverChange.pointLight[0].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCBNeverChange.pointLight[0].Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mCBNeverChange.pointLight[0].Range = 25.0f;
	mCBNeverChange.numDirLight = 1;
	mCBNeverChange.numPointLight = 1;
	mCBNeverChange.numSpotLight = 0;
	


	// 更新不容易被修改的常量缓冲区资源
	mBasicFX.UpdateConstantBuffer(mCBOnReSize);
	mBasicFX.UpdateConstantBuffer(mCBNeverChange);


	return true;
}



