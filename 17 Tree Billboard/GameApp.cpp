#include "GameApp.h"
#include <filesystem>
#include <algorithm>
using namespace DirectX;
using namespace std::experimental;
using namespace Microsoft::WRL;

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

	// 只有常量缓冲区被初始化后才执行更新操作
	if (mBasicFX.IsInit())
	{
		mCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
		mCBChangesOnReSize.proj = mCamera->GetViewXM();
		mBasicFX.UpdateConstantBuffer(mCBChangesOnReSize);
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
		// 自由摄像机的操作

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

	// 将位置限制在[-49.9f, 49.9f]的区域内
	// 不允许穿地
	XMFLOAT3 adjustedPos;
	XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), 
		XMVectorSet(-49.9f, 0.0f, -49.9f, 0.0f), XMVectorSet(49.9f, 99.9f, 49.9f, 0.0f)));
	cam1st->SetPosition(adjustedPos);

	// 更新观察矩阵
	mCamera->UpdateViewMatrix();
	XMStoreFloat4(&mCBChangesEveryFrame.eyePos, mCamera->GetPositionXM());
	mCBChangesEveryFrame.view = mCamera->GetViewXM();

	bool isDrawingStateChanged = false;
	// 开关雾效
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		mCBDrawingStates.fogEnabled = !mCBDrawingStates.fogEnabled;
		isDrawingStateChanged = true;
	}
	// 白天/黑夜变换
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		mIsNight = !mIsNight;
		if (mIsNight)
		{
			// 黑夜模式下变为逐渐黑暗
			mCBDrawingStates.fogColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
			mCBDrawingStates.fogStart = 5.0f;
		}
		else
		{
			// 白天模式则对应雾效
			mCBDrawingStates.fogColor = XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f);
			mCBDrawingStates.fogStart = 15.0f;
		}
		isDrawingStateChanged = true;
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		mEnableAlphaToCoverage = !mEnableAlphaToCoverage;
	}
	// 调整雾的范围
	if (mouseState.scrollWheelValue != 0)
	{
		// 一次滚轮滚动的最小单位为120
		mCBDrawingStates.fogRange += mouseState.scrollWheelValue / 120;
		if (mCBDrawingStates.fogRange < 15.0f)
			mCBDrawingStates.fogRange = 15.0f;
		else if (mCBDrawingStates.fogRange > 175.0f)
			mCBDrawingStates.fogRange = 175.0f;
		isDrawingStateChanged = true;
	}
	

	// 重置滚轮值
	mMouse->ResetScrollWheelValue();

	

	// 退出程序，这里应向窗口发送销毁信息
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);

	if (isDrawingStateChanged)
	{
		mBasicFX.UpdateConstantBuffer(mCBDrawingStates);
	}
	mBasicFX.UpdateConstantBuffer(mCBChangesEveryFrame);



}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	// 白天/黑夜设置
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
	mBasicFX.SetRenderDefault();
	mGround.Draw(md3dImmediateContext);

	// 绘制树
	mBasicFX.SetRenderBillboard(mEnableAlphaToCoverage);
	UINT stride = sizeof(VertexPosSize);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, mPointSpritesBuffer.GetAddressOf(), &stride, &offset);
	md3dImmediateContext->Draw(16, 0);

	//
	// 绘制Direct2D部分
	//
	md2dRenderTarget->BeginDraw();
	std::wstring text = L"1-雾效开关 2-白天/黑夜雾效切换 3-AlphaToCoverage开关 Esc-退出\n"
		"滚轮-调整雾效范围\n"
		"仅支持自由视角摄像机\n";
	text += std::wstring(L"AlphaToCoverage状态: ") + (mEnableAlphaToCoverage ? L"开启\n" : L"关闭\n");
	text += std::wstring(L"雾效状态: ") + (mCBDrawingStates.fogEnabled ? L"开启\n" : L"关闭\n");
	if (mCBDrawingStates.fogEnabled)
	{
		text += std::wstring(L"天气情况: ") + (mIsNight ? L"黑夜\n" : L"白天\n");
		text += L"雾效范围: " + std::to_wstring(mIsNight ? 5 : 15) + L"-" + 
			std::to_wstring((mIsNight ? 5 : 15) + (int)mCBDrawingStates.fogRange);
	}


	md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
		D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
	HR(md2dRenderTarget->EndDraw());

	HR(mSwapChain->Present(0, 0));

}



bool GameApp::InitResource()
{
	// 默认白天，开启AlphaToCoverage
	mIsNight = false;
	mEnableAlphaToCoverage = true;
	// ******************
	// 初始化各种物体

	// 初始化树纹理资源
	mTreeTexArray = CreateDDSTexture2DArrayShaderResourceView(
		md3dDevice,
		md3dImmediateContext,
		std::vector<std::wstring>{
		L"Texture\\tree0.dds",
			L"Texture\\tree1.dds",
			L"Texture\\tree2.dds",
			L"Texture\\tree3.dds"});
	
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
	// 初始化常量缓冲区的值

	mCBChangesEveryDrawing.material = mTreeMat;
	mCBChangesEveryDrawing.world = mCBChangesEveryDrawing.worldInvTranspose = XMMatrixIdentity();
	mCBChangesEveryDrawing.texTransform = XMMatrixIdentity();


	// 方向光
	mCBRarely.dirLight[0].Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	mCBRarely.dirLight[0].Diffuse = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	mCBRarely.dirLight[0].Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	mCBRarely.dirLight[0].Direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	mCBRarely.dirLight[1] = mCBRarely.dirLight[0];
	mCBRarely.dirLight[1].Direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
	mCBRarely.dirLight[2] = mCBRarely.dirLight[0];
	mCBRarely.dirLight[2].Direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
	mCBRarely.dirLight[3] = mCBRarely.dirLight[0];
	mCBRarely.dirLight[3].Direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);

	// 摄像机相关
	mCameraMode = CameraMode::Free;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	mCamera = camera;
	camera->SetPosition(XMFLOAT3());
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();


	mCBChangesEveryFrame.view = camera->GetViewXM();
	XMStoreFloat4(&mCBChangesEveryFrame.eyePos, camera->GetPositionXM());

	mCBChangesOnReSize.proj = camera->GetProjXM();
	
	// 雾状态默认开启
	mCBDrawingStates.fogEnabled = 1;
	mCBDrawingStates.fogColor = XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f);	// 银色
	mCBDrawingStates.fogRange = 75.0f;
	mCBDrawingStates.fogStart = 15.0f;
	// 更新常量缓冲区资源
	mBasicFX.UpdateConstantBuffer(mCBChangesEveryDrawing);
	mBasicFX.UpdateConstantBuffer(mCBChangesEveryFrame);
	mBasicFX.UpdateConstantBuffer(mCBChangesOnReSize);
	mBasicFX.UpdateConstantBuffer(mCBDrawingStates);
	mBasicFX.UpdateConstantBuffer(mCBRarely);

	// 直接绑定树的纹理
	md3dImmediateContext->PSSetShaderResources(1, 1, mTreeTexArray.GetAddressOf());
	
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


ComPtr<ID3D11ShaderResourceView> GameApp::CreateDDSTexture2DArrayShaderResourceView(
	ComPtr<ID3D11Device> device,
	ComPtr<ID3D11DeviceContext> deviceContext,
	const std::vector<std::wstring>& filenames,
	int maxMipMapSize)
{
	//
	// 1. 读取所有纹理
	//
	size_t size = filenames.size();
	std::vector<ComPtr<ID3D11Texture2D>> srcTex(size);
	UINT mipLevel = maxMipMapSize;
	UINT width, height;
	DXGI_FORMAT format;
	for (size_t i = 0; i < size; ++i)
	{
		// 由于这些纹理并不会被GPU使用，我们使用D3D11_USAGE_STAGING枚举值
		// 使得CPU可以读取资源
		HR(CreateDDSTextureFromFileEx(device.Get(),
			deviceContext.Get(),
			filenames[i].c_str(),
			maxMipMapSize,
			D3D11_USAGE_STAGING,							// Usage
			0,												// BindFlags
			D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,	// CpuAccessFlags
			0,												// MiscFlags
			false,
			(ID3D11Resource**)srcTex[i].GetAddressOf(),
			nullptr));

		// 读取创建好的纹理Mipmap等级, 宽度和高度
		D3D11_TEXTURE2D_DESC texDesc;
		srcTex[i]->GetDesc(&texDesc);
		if (i == 0)
		{
			mipLevel = texDesc.MipLevels;
			width = texDesc.Width;
			height = texDesc.Height;
			format = texDesc.Format;
		}
		// 这里断言所有纹理的MipMap等级，宽度和高度应当一致
		assert(mipLevel == texDesc.MipLevels);
		assert(texDesc.Width == width && texDesc.Height == height);
		// 这里要求所有提供的图片数据格式应当是一致的，若存在不一致的情况，请
		// 使用dxtex.exe(DirectX Texture Tool)将所有的图片转成一致的数据格式
		assert(texDesc.Format == format);
		
	}

	//
	// 2.创建纹理数组
	//
	D3D11_TEXTURE2D_DESC texDesc, texArrayDesc;
	srcTex[0]->GetDesc(&texDesc);
	texArrayDesc.Width = texDesc.Width;
	texArrayDesc.Height = texDesc.Height;
	texArrayDesc.MipLevels = texDesc.MipLevels;
	texArrayDesc.ArraySize = size;
	texArrayDesc.Format = texDesc.Format;
	texArrayDesc.SampleDesc.Count = 1;
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;
	texArrayDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> texArray;
	HR(device->CreateTexture2D(&texArrayDesc, nullptr, texArray.GetAddressOf()));

	//
	// 3.将所有的纹理子资源赋值到纹理数组中
	//

	// 每个纹理元素
	for (size_t i = 0; i < size; ++i)
	{
		// 纹理中的每个mipmap等级
		for (UINT j = 0; j < mipLevel; ++j)
		{
			D3D11_MAPPED_SUBRESOURCE mappedTex2D;
			// 允许映射索引i纹理中，索引j的mipmap等级的2D纹理
			HR(deviceContext->Map(srcTex[i].Get(),
				j, D3D11_MAP_READ, 0, &mappedTex2D));
			deviceContext->UpdateSubresource(
				texArray.Get(),
				D3D11CalcSubresource(j, i, mipLevel),	// i * mipLevel + j
				nullptr,
				mappedTex2D.pData,
				mappedTex2D.RowPitch,
				mappedTex2D.DepthPitch);
			// 停止映射
			deviceContext->Unmap(srcTex[i].Get(), j);
		}
	}

	//
	// 4.创建纹理数组的SRV
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texArrayDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesc.Texture2DArray.MostDetailedMip = 0;
	viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
	viewDesc.Texture2DArray.FirstArraySlice = 0;
	viewDesc.Texture2DArray.ArraySize = size;

	ComPtr<ID3D11ShaderResourceView> texArraySRV;
	HR(device->CreateShaderResourceView(texArray.Get(), &viewDesc, texArraySRV.GetAddressOf()));


	// 已经确保所有资源由ComPtr管理，无需手动释放

	return texArraySRV;
}


