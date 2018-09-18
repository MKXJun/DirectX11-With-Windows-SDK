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
	mMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

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

	// 更新投影矩阵
	mBasicObjectFX.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f));

}

void GameApp::UpdateScene(float dt)
{

	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = mMouse->GetState();
	Mouse::State lastMouseState = mMouseTracker.GetLastState();
	mMouseTracker.Update(mouseState);

	Keyboard::State keyState = mKeyboard->GetState();
	mKeyboardTracker.Update(keyState);

	// 更新每帧变化的值
	UINT stride = (mShowMode != Mode::SplitedSphere ? sizeof(VertexPosColor) : sizeof(VertexPosNormalColor));
	UINT offset = 0;



	// 切换分形
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Q))
	{
		mShowMode = Mode::SplitedTriangle;
		ResetSplitedTriangle();
		mIsWireFrame = false;
		mShowNormal = false;
		mCurrIndex = 0;
		stride = sizeof(VertexPosColor);
		md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::W))
	{
		mShowMode = Mode::SplitedSnow;
		ResetSplitedSnow();
		mIsWireFrame = true;
		mShowNormal = false;
		mCurrIndex = 0;
		stride = sizeof(VertexPosColor);
		md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::E))
	{
		mShowMode = Mode::SplitedSphere;
		ResetSplitedSphere();
		mIsWireFrame = false;
		mShowNormal = false;
		mCurrIndex = 0;
		stride = sizeof(VertexPosNormalColor);
		md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}

	// 切换阶数
	for (int i = 0; i < 7; ++i)
	{
		if (mKeyboardTracker.IsKeyPressed((Keyboard::Keys)((int)Keyboard::D1 + i)))
		{
			md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[i].GetAddressOf(), &stride, &offset);
			mCurrIndex = i;
		}
	}

	// 切换线框/面
	if (mKeyboardTracker.IsKeyPressed(Keyboard::M))
	{
		if (mShowMode != Mode::SplitedSnow)
		{
			mIsWireFrame = !mIsWireFrame;
		}
	}

	// 是否添加法向量
	if (mKeyboardTracker.IsKeyPressed(Keyboard::N))
	{
		if (mShowMode == Mode::SplitedSphere)
		{
			mShowNormal = !mShowNormal;
		}
	}

	// 让球体转起来
	if (mShowMode == Mode::SplitedSphere)
	{
		static float theta = 0.0f;
		theta += 0.3f * dt;
		mBasicObjectFX.SetWorldMatrix(XMMatrixRotationY(theta));
	}
	else
	{
		mBasicObjectFX.SetWorldMatrix(XMMatrixIdentity());
	}

}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);


	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	// 根据当前绘制模式设置需要用于渲染的各项资源
	if (mShowMode == Mode::SplitedTriangle)
	{
		mBasicObjectFX.SetRenderSplitedTriangle(md3dImmediateContext);
	}
	else if (mShowMode == Mode::SplitedSnow)
	{
		mBasicObjectFX.SetRenderSplitedSnow(md3dImmediateContext);
	}
	else if (mShowMode == Mode::SplitedSphere)
	{
		mBasicObjectFX.SetRenderSplitedSphere(md3dImmediateContext);
	}

	// 设置线框/面模式
	if (mIsWireFrame)
	{
		md3dImmediateContext->RSSetState(RenderStates::RSWireframe.Get());
	}
	else
	{
		md3dImmediateContext->RSSetState(nullptr);
	}

	// 进行绘制，记得应用常量缓冲区的变更
	mBasicObjectFX.Apply(md3dImmediateContext);
	md3dImmediateContext->Draw(mVertexCounts[mCurrIndex], 0);
	// 绘制法向量
	if (mShowNormal)
	{
		mBasicObjectFX.SetRenderNormal(md3dImmediateContext);
		mBasicObjectFX.Apply(md3dImmediateContext);
		md3dImmediateContext->Draw(mVertexCounts[mCurrIndex], 0);
	}


	//
	// 绘制Direct2D部分
	//
	md2dRenderTarget->BeginDraw();
	std::wstring text = L"切换分形：Q-三角形(面/线框) W-雪花(线框) E-球(面/线框)\n"
		L"主键盘数字1 - 7：分形阶数，越高越精细\n"
		L"M-面/线框切换\n\n"
		L"当前阶数: " + std::to_wstring(mCurrIndex + 1) + L"\n"
		"当前分形: ";
	if (mShowMode == Mode::SplitedTriangle)
		text += L"三角形";
	else if (mShowMode == Mode::SplitedSnow)
		text += L"雪花";
	else
		text += L"球";

	if (mIsWireFrame)
		text += L"(线框)";
	else
		text += L"(面)";

	if (mShowMode == Mode::SplitedSphere)
	{
		if (mShowNormal)
			text += L"(N-关闭法向量)";
		else
			text += L"(N-开启法向量)";
	}



	md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
		D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
	HR(md2dRenderTarget->EndDraw());

	HR(mSwapChain->Present(0, 0));

}



bool GameApp::InitResource()
{
	// 默认绘制三角形
	mShowMode = Mode::SplitedTriangle;
	mIsWireFrame = false;
	mShowNormal = false;
	ResetSplitedTriangle();
	// 预先绑定顶点缓冲区
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffers[0].GetAddressOf(), &stride, &offset);


	// ******************
	// 初始化常量缓冲区的值
	// 方向光
	DirectionalLight dirLight;
	dirLight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.Direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	mBasicObjectFX.SetDirLight(0, dirLight);
	// 材质
	Material material;
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
	mBasicObjectFX.SetMaterial(material);
	// 摄像机位置
	mBasicObjectFX.SetEyePos(XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f));
	// 矩阵
	mBasicObjectFX.SetWorldViewProjMatrix(
		XMMatrixIdentity(),
		XMMatrixLookAtLH(
			XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f),
			XMVectorZero(),
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
		XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));

	mBasicObjectFX.SetSphereCenter(XMFLOAT3());
	mBasicObjectFX.SetSphereRadius(2.0f);

	return true;
}


void GameApp::ResetSplitedTriangle()
{
	// ******************
	// 初始化三角形
	// 设置三角形顶点
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.0f * 3, 0.866f * 3, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};
	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// 需要额外添加流输出标签
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mVertexBuffers[0].ReleaseAndGetAddressOf()));


	// 三角形顶点数
	mVertexCounts[0] = 3;
	// 初始化所有顶点缓冲区
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 3;
		mVertexCounts[i] = mVertexCounts[i - 1] * 3;
		HR(md3dDevice->CreateBuffer(&vbd, nullptr, mVertexBuffers[i].ReleaseAndGetAddressOf()));
		mBasicObjectFX.SetStreamOutputSplitedTriangle(md3dImmediateContext, mVertexBuffers[i - 1], mVertexBuffers[i]);
		md3dImmediateContext->Draw(mVertexCounts[i - 1], 0);
	}
}

void GameApp::ResetSplitedSnow()
{
	// ******************
	// 雪花分形从初始化三角形开始，需要6个顶点
	// 设置三角形顶点
	float sqrt3 = sqrt(3.0f);
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(0.0f, sqrt3 / 2, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(0.0f, sqrt3 / 2, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(-3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
	};
	// 将三角形宽度和高度都放大3倍
	for (VertexPosColor& v : vertices)
	{
		v.pos.x *= 3;
		v.pos.y *= 3;
	}

	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// 需要额外添加流输出标签
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mVertexBuffers[0].ReleaseAndGetAddressOf()));

	// 顶点数
	mVertexCounts[0] = 6;
	// 初始化所有顶点缓冲区
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 4;
		mVertexCounts[i] = mVertexCounts[i - 1] * 4;
		HR(md3dDevice->CreateBuffer(&vbd, nullptr, mVertexBuffers[i].ReleaseAndGetAddressOf()));
		mBasicObjectFX.SetStreamOutputSplitedSnow(md3dImmediateContext, mVertexBuffers[i - 1], mVertexBuffers[i]);
		md3dImmediateContext->Draw(mVertexCounts[i - 1], 0);
	}
}

void GameApp::ResetSplitedSphere()
{
	VertexPosNormalColor basePoint[] = {
		{ XMFLOAT3(0.0f, 2.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(2.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(-2.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(0.0f, 0.0f, -2.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(0.0f, -2.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	};
	int indices[] = { 0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 1, 4, 1, 2, 5, 2, 3, 5, 3, 4, 5, 4, 1, 5 };

	std::vector<VertexPosNormalColor> vertices;
	for (int pos : indices)
	{
		vertices.push_back(basePoint[pos]);
	}


	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = (UINT)(vertices.size() * sizeof(VertexPosNormalColor));
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// 需要额外添加流输出标签
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mVertexBuffers[0].ReleaseAndGetAddressOf()));

	// 顶点数
	mVertexCounts[0] = 24;
	// 初始化所有顶点缓冲区
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 4;
		mVertexCounts[i] = mVertexCounts[i - 1] * 4;
		HR(md3dDevice->CreateBuffer(&vbd, nullptr, mVertexBuffers[i].ReleaseAndGetAddressOf()));
		mBasicObjectFX.SetStreamOutputSplitedSphere(md3dImmediateContext, mVertexBuffers[i - 1], mVertexBuffers[i]);
		md3dImmediateContext->Draw(mVertexCounts[i - 1], 0);
	}
}



