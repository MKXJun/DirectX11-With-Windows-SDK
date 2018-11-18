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

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(md3dDevice);

	if (!mBasicEffect.InitAll(md3dDevice))
		return false;

	if (!mScreenFadeEffect.InitAll(md3dDevice))
		return false;

	if (!mMinimapEffect.InitAll(md3dDevice))
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
		// 小地图网格模型重设
		mMinimap.SetMesh(md3dDevice, Geometry::Create2DShow(1.0f - 100.0f / mClientWidth * 2,  -1.0f + 100.0f / mClientHeight * 2, 
			100.0f / mClientWidth * 2, 100.0f / mClientHeight * 2));
		// 屏幕淡入淡出纹理大小重设
		mScreenFadeRender = std::make_unique<TextureRender>(md3dDevice, mClientWidth, mClientHeight, false);
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

	// 更新淡入淡出值，并限制摄像机行动
	if (mFadeUsed)
	{
		mFadeAmount += mFadeSign * dt / 2.0f;	// 2s时间淡入/淡出
		if (mFadeSign > 0.0f && mFadeAmount > 1.0f)
		{
			mFadeAmount = 1.0f;
			mFadeUsed = false;	// 结束淡入
		}
		else if (mFadeSign < 0.0f && mFadeAmount < 0.0f)
		{
			mFadeAmount = 0.0f;
			SendMessage(MainWnd(), WM_DESTROY, 0, 0);	// 关闭程序
			// 这里不结束淡出是因为发送关闭窗口的消息还要过一会才真正关闭
		}
	}
	else
	{
		// ********************
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
	mCamera->UpdateViewMatrix();
	mBasicEffect.SetViewMatrix(mCamera->GetViewXM());
	mBasicEffect.SetEyePos(mCamera->GetPositionXM());
	mMinimapEffect.SetEyePos(mCamera->GetPositionXM());
	
	// 截屏
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Q))
		mPrintScreenStarted = true;
		
	// 重置滚轮值
	mMouse->ResetScrollWheelValue();

	// 退出程序，开始淡出
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Escape))
	{
		mFadeSign = -1.0f;
		mFadeUsed = true;
	}
}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	
	// ******************
	// 绘制Direct3D部分
	//

	// 预先清空后备缓冲区
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	if (mFadeUsed)
	{
		// 开始淡入/淡出
		mScreenFadeRender->Begin(md3dImmediateContext);
	}


	// 绘制主场景
	DrawScene(false);

	UINT strides[1] = { sizeof(VertexPosNormalTex) };
	UINT offsets[1] = { 0 };
	
	// 小地图特效应用
	mMinimapEffect.SetRenderDefault(md3dImmediateContext);
	mMinimapEffect.Apply(md3dImmediateContext);
	// 最后绘制小地图
	md3dImmediateContext->IASetVertexBuffers(0, 1, mMinimap.modelParts[0].vertexBuffer.GetAddressOf(), strides, offsets);
	md3dImmediateContext->IASetIndexBuffer(mMinimap.modelParts[0].indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	md3dImmediateContext->DrawIndexed(6, 0, 0);

	if (mFadeUsed)
	{
		// 结束淡入/淡出，此时绘制的场景在屏幕淡入淡出渲染的纹理
		mScreenFadeRender->End(md3dImmediateContext);

		// 屏幕淡入淡出特效应用
		mScreenFadeEffect.SetRenderDefault(md3dImmediateContext);
		mScreenFadeEffect.SetFadeAmount(mFadeAmount);
		mScreenFadeEffect.SetTexture(mScreenFadeRender->GetOutputTexture());
		mScreenFadeEffect.SetWorldViewProjMatrix(XMMatrixIdentity());
		mScreenFadeEffect.Apply(md3dImmediateContext);
		// 将保存的纹理输出到屏幕
		md3dImmediateContext->IASetVertexBuffers(0, 1, mFullScreenShow.modelParts[0].vertexBuffer.GetAddressOf(), strides, offsets);
		md3dImmediateContext->IASetIndexBuffer(mFullScreenShow.modelParts[0].indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		md3dImmediateContext->DrawIndexed(6, 0, 0);
	}
	
	// 若截屏键Q按下，则分别保存到output.dds和output.png中
	if (mPrintScreenStarted)
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		// 输出截屏
		mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
		HR(SaveDDSTextureToFile(md3dImmediateContext.Get(), backBuffer.Get(), L"Screenshot\\output.dds"));
		HR(SaveWICTextureToFile(md3dImmediateContext.Get(), backBuffer.Get(), GUID_ContainerFormatPng, L"Screenshot\\output.png"));
		// 结束截屏
		mPrintScreenStarted = false;
	}



	// ******************
	// 绘制Direct2D部分
	//
	md2dRenderTarget->BeginDraw();
	std::wstring text = L"当前摄像机模式: 第一人称  Esc退出\n"
		"鼠标移动控制视野 W/S/A/D移动\n"
		"Q-截屏(输出output.dds和output.png到ScreenShot文件夹)";

	

	md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
		D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
	HR(md2dRenderTarget->EndDraw());

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	mPrintScreenStarted = false;
	mFadeUsed = true;	// 开始淡入
	mFadeAmount = 0.0f;
	mFadeSign = 1.0f;


	// ******************
	// 初始化用于Render-To-Texture的对象
	//
	mMinimapRender = std::make_unique<TextureRender>(md3dDevice, 400, 400, true);
	mScreenFadeRender = std::make_unique<TextureRender>(md3dDevice, mClientWidth, mClientHeight, false);

	// ******************
	// 初始化游戏对象
	//

	// 创建随机的树
	CreateRandomTrees();

	// 初始化地面
	mObjReader.Read(L"Model\\ground.mbo", L"Model\\ground.obj");
	mGround.SetModel(Model(md3dDevice, mObjReader));

	// 初始化网格，放置在右下角200x200
	mMinimap.SetMesh(md3dDevice, Geometry::Create2DShow(0.75f, -0.66666666f, 0.25f, 0.33333333f));
	
	// 覆盖整个屏幕面的网格模型
	mFullScreenShow.SetMesh(md3dDevice, Geometry::Create2DShow());

	// ******************
	// 初始化摄像机
	//

	// 默认摄像机
	mCameraMode = CameraMode::FirstPerson;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();
	

	// 小地图摄像机
	mMinimapCamera = std::unique_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mMinimapCamera->SetViewPort(0.0f, 0.0f, 200.0f, 200.0f);	// 200x200小地图
	mMinimapCamera->LookTo(
		XMVectorSet(0.0f, 10.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, -1.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
	mMinimapCamera->UpdateViewMatrix();

	// ******************
	// 初始化几乎不会变化的值
	//

	// 黑夜特效
	mBasicEffect.SetFogColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	mBasicEffect.SetFogStart(5.0f);
	mBasicEffect.SetFogRange(20.0f);

	// 小地图范围可视
	mMinimapEffect.SetFogState(true);
	mMinimapEffect.SetInvisibleColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	mMinimapEffect.SetMinimapRect(XMVectorSet(-95.0f, 95.0f, 95.0f, -95.0f));
	mMinimapEffect.SetVisibleRange(25.0f);

	// 方向光(默认)
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

	// ******************
	// 渲染小地图纹理
	// 

	mBasicEffect.SetViewMatrix(mMinimapCamera->GetViewXM());
	mBasicEffect.SetProjMatrix(XMMatrixOrthographicLH(190.0f, 190.0f, 1.0f, 20.0f));	// 使用正交投影矩阵(中心在摄像机位置)
	// 关闭雾效
	mBasicEffect.SetFogState(false);
	mMinimapRender->Begin(md3dImmediateContext);
	DrawScene(true);
	mMinimapRender->End(md3dImmediateContext);

	mMinimapEffect.SetTexture(mMinimapRender->GetOutputTexture());


	// 开启雾效，恢复投影矩阵并设置偏暗的光照
	mBasicEffect.SetFogState(true);
	mBasicEffect.SetProjMatrix(mCamera->GetProjXM());
	dirLight[0].Ambient = XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f);
	dirLight[0].Diffuse = XMFLOAT4(0.16f, 0.16f, 0.16f, 1.0f);
	for (int i = 0; i < 4; ++i)
		mBasicEffect.SetDirLight(i, dirLight[i]);
	

	return true;
}

void GameApp::DrawScene(bool drawMinimap)
{

	mBasicEffect.SetTextureUsed(true);


	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
	if (drawMinimap)
	{
		// 小地图下绘制所有树
		mTrees.DrawInstanced(md3dImmediateContext, mBasicEffect, mInstancedData);
	}
	else
	{
		// 统计实际绘制的物体数目
		std::vector<XMMATRIX> acceptedData;
		// 默认视锥体裁剪
		acceptedData = Collision::FrustumCulling(mInstancedData, mTrees.GetLocalBoundingBox(),
			mCamera->GetViewXM(), mCamera->GetProjXM());
		// 默认硬件实例化绘制
		mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderInstance);
		mTrees.DrawInstanced(md3dImmediateContext, mBasicEffect, acceptedData);
	}
	
	// 绘制地面
	mBasicEffect.SetRenderDefault(md3dImmediateContext, BasicEffect::RenderObject);
	mGround.Draw(md3dImmediateContext, mBasicEffect);	
}

void GameApp::CreateRandomTrees()
{
	srand((unsigned)time(nullptr));
	// 初始化树
	mObjReader.Read(L"Model\\tree.mbo", L"Model\\tree.obj");
	mTrees.SetModel(Model(md3dDevice, mObjReader));
	XMMATRIX S = XMMatrixScaling(0.015f, 0.015f, 0.015f);

	BoundingBox treeBox = mTrees.GetLocalBoundingBox();
	// 获取树包围盒顶点
	mTreeBoxData = Collision::CreateBoundingBox(treeBox, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
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
				mInstancedData.push_back(World);
			}
		}
		theta += XM_2PI / 16;
	}
}