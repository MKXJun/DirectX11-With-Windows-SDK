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

	// 法线贴图开关
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
		mEnableNormalMap = !mEnableNormalMap;
		
	// 切换地面纹理
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D2) && mGroundMode != GroundMode::Floor)
	{
		mGroundMode = GroundMode::Floor;
		mGroundModel.modelParts[0].texDiffuse = mFloorDiffuse;
		mGroundTModel.modelParts[0].texDiffuse = mFloorDiffuse;
		mGround.SetModel(mGroundModel);
		mGroundT.SetModel(mGroundTModel);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::D3) && mGroundMode != GroundMode::Stones)
	{
		mGroundMode = GroundMode::Stones;
		mGroundModel.modelParts[0].texDiffuse = mStonesDiffuse;
		mGroundTModel.modelParts[0].texDiffuse = mStonesDiffuse;
		mGround.SetModel(mGroundModel);
		mGroundT.SetModel(mGroundTModel);
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
	// 生成动态天空盒
	//

	// 保留当前绘制的渲染目标视图和深度模板视图
	mDaylight->Cache(md3dImmediateContext, mBasicEffect);

	// 绘制动态天空盒的每个面（以球体为中心）
	for (int i = 0; i < 6; ++i)
	{
		mDaylight->BeginCapture(
			md3dImmediateContext, mBasicEffect, XMFLOAT3(0.0f, 0.0f, 0.0f), static_cast<D3D11_TEXTURECUBE_FACE>(i));

		// 不绘制中心球
		DrawScene(false);
	}

	// 恢复之前的绘制设定
	mDaylight->Restore(md3dImmediateContext, mBasicEffect, *mCamera);
	
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
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式: 自由视角  Esc退出\n"
			"鼠标移动控制视野 W/S/A/D移动\n"
			"1-法线贴图: ";
		text += mEnableNormalMap ? L"开启\n" : L"关闭\n";
		text += L"切换纹理: 2-地板  3-鹅卵石面\n"
			"当前纹理: ";
		text += (mGroundMode == GroundMode::Floor ? L"地板" : L"鹅卵石面");

		md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
		HR(md2dRenderTarget->EndDraw());
	}

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// 初始化法线贴图相关
	//
	mEnableNormalMap = true;

	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\bricks_nmap.dds", nullptr, mBricksNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\floor_nmap.dds", nullptr, mFloorNormalMap.GetAddressOf()));
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\stones_nmap.dds", nullptr, mStonesNormalMap.GetAddressOf()));

	// ******************
	// 初始化天空盒相关

	mDaylight = std::make_unique<DynamicSkyRender>(
		md3dDevice, md3dImmediateContext, 
		L"Texture\\daylight.jpg", 
		5000.0f, 256);

	mBasicEffect.SetTextureCube(mDaylight->GetDynamicTextureCube());

	// ******************
	// 初始化游戏对象
	//

	mGroundMode = GroundMode::Floor;
	
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\floor.dds", nullptr, mFloorDiffuse.GetAddressOf()));
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\stones.dds", nullptr, mStonesDiffuse.GetAddressOf()));
	// 地面
	mGroundModel.SetMesh(md3dDevice,
		Geometry::CreatePlane(XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	mGroundModel.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mGroundModel.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mGroundModel.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	mGroundModel.modelParts[0].material.Reflect = XMFLOAT4();
	mGroundModel.modelParts[0].texDiffuse = mFloorDiffuse;
	mGround.SetModel(mGroundModel);

	// 带切线向量的地面
	mGroundTModel.SetMesh(md3dDevice, Geometry::CreatePlane<VertexPosNormalTangentTex>(
		XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT2(16.0f, 16.0f), XMFLOAT2(8.0f, 8.0f)));
	mGroundTModel.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mGroundTModel.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mGroundTModel.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	mGroundTModel.modelParts[0].material.Reflect = XMFLOAT4();
	mGroundTModel.modelParts[0].texDiffuse = mFloorDiffuse;
	mGroundT.SetModel(mGroundTModel);

	// 球体
	Model model;
	ComPtr<ID3D11ShaderResourceView> texDiffuse;

	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Texture\\stone.dds",
		nullptr,
		texDiffuse.GetAddressOf()));

	model.SetMesh(md3dDevice, Geometry::CreateSphere(1.0f, 30, 30));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].texDiffuse = texDiffuse;
	mSphere.SetModel(std::move(model));

	// 柱体
	HR(CreateDDSTextureFromFile(md3dDevice.Get(),
		L"Texture\\bricks.dds",
		nullptr,
		texDiffuse.ReleaseAndGetAddressOf()));

	model.SetMesh(md3dDevice,
		Geometry::CreateCylinder(0.5f, 2.0f));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	model.modelParts[0].texDiffuse = texDiffuse;
	mCylinder.SetModel(std::move(model));

	// 带切线向量的柱体
	model.SetMesh(md3dDevice,
		Geometry::CreateCylinder<VertexPosNormalTangentTex>(0.5f, 2.0f));
	model.modelParts[0].material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.Reflect = XMFLOAT4();
	model.modelParts[0].texDiffuse = texDiffuse;
	mCylinderT.SetModel(std::move(model));

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
	dirLight[0].Diffuse = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
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
	
	// 只绘制球体的反射效果
	if (drawCenterSphere)
	{
		mBasicEffect.SetReflectionEnabled(true);
		mBasicEffect.SetRefractionEnabled(false);
		mSphere.Draw(md3dImmediateContext, mBasicEffect);
	}
	
	// 绘制地面
	mBasicEffect.SetReflectionEnabled(false);
	mBasicEffect.SetRefractionEnabled(false);
	
	if (mEnableNormalMap)
	{
		mBasicEffect.SetRenderWithNormalMap(md3dImmediateContext, BasicEffect::RenderObject);
		if (mGroundMode == GroundMode::Floor)
			mBasicEffect.SetTextureNormalMap(mFloorNormalMap);
		else
			mBasicEffect.SetTextureNormalMap(mStonesNormalMap);
		mGroundT.Draw(md3dImmediateContext, mBasicEffect);
	}
	else
	{
		mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
		mGround.Draw(md3dImmediateContext, mBasicEffect);
	}

	// 绘制五个圆柱
	// 需要固定位置
	static std::vector<XMMATRIX> cyliderWorlds = {
		XMMatrixTranslation(0.0f, -1.99f, 0.0f),
		XMMatrixTranslation(4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, 4.5f),
		XMMatrixTranslation(-4.5f, -1.99f, -4.5f),
		XMMatrixTranslation(4.5f, -1.99f, -4.5f)
	};

	if (mEnableNormalMap)
	{
		mBasicEffect.SetRenderWithNormalMap(md3dImmediateContext, BasicEffect::RenderInstance);
		mBasicEffect.SetTextureNormalMap(mBricksNormalMap);
		mCylinderT.DrawInstanced(md3dImmediateContext, mBasicEffect, cyliderWorlds);
	}
	else
	{
		mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
		mCylinder.DrawInstanced(md3dImmediateContext, mBasicEffect, cyliderWorlds);
	}
	
	// 绘制五个圆球
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
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

	if (drawCenterSphere)
		mDaylight->Draw(md3dImmediateContext, mSkyEffect, *mCamera);
	else
		mDaylight->Draw(md3dImmediateContext, mSkyEffect, mDaylight->GetCamera());
	
}

