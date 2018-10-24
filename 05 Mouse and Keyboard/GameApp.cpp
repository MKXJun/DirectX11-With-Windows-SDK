#include "GameApp.h"
#include "d3dUtil.h"
using namespace DirectX;
using namespace std::experimental;

const D3D11_INPUT_ELEMENT_DESC GameApp::VertexPosColor::inputLayout[2] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

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

	if (!InitEffect())
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
	D3DApp::OnResize();
}

void GameApp::UpdateScene(float dt)
{
	// 更新常量缓冲区，让立方体转起来
	static float cubePhi = 0.0f, cubeTheta = 0.0f;
	// 获取鼠标状态
	Mouse::State mouseState = mMouse->GetState();
	Mouse::State lastMouseState = mMouseTracker.GetLastState();
	// 获取键盘状态
	Keyboard::State keyState = mKeyboard->GetState();
	Keyboard::State lastKeyState = mKeyboardTracker.GetLastState();

	// 更新鼠标按钮状态跟踪器，仅当鼠标按住的情况下才进行移动
	mMouseTracker.Update(mouseState);
	mKeyboardTracker.Update(keyState);
	if (mouseState.leftButton == true && mMouseTracker.leftButton == mMouseTracker.HELD)
	{
		cubeTheta -= (mouseState.x - lastMouseState.x) * 0.01f;
		cubePhi -= (mouseState.y - lastMouseState.y) * 0.01f;
	}
	if (keyState.IsKeyDown(Keyboard::W))
		cubePhi += dt * 2;
	if (keyState.IsKeyDown(Keyboard::S))
		cubePhi -= dt * 2;
	if (keyState.IsKeyDown(Keyboard::A))
		cubeTheta += dt * 2;
	if (keyState.IsKeyDown(Keyboard::D))
		cubeTheta -= dt * 2;


	mCBuffer.world = XMMatrixTranspose(XMMatrixRotationY(cubeTheta) * XMMatrixRotationX(cubePhi));
	md3dImmediateContext->UpdateSubresource(mConstantBuffer.Get(), 0, nullptr, &mCBuffer, 0, 0);
}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	static float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };	// RGBA = (0,0,0,255)
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 绘制立方体
	md3dImmediateContext->DrawIndexed(36, 0, 0);
	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitEffect()
{
	ComPtr<ID3DBlob> blob;


	// 创建顶点着色器
	HR(CreateShaderFromFile(L"HLSL\\Cube_VS.vso", L"HLSL\\Cube_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(md3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mVertexShader.GetAddressOf()));
	// 创建并绑定顶点布局
	HR(md3dDevice->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), mVertexLayout.GetAddressOf()));

	// 创建像素着色器
	HR(CreateShaderFromFile(L"HLSL\\Cube_PS.pso", L"HLSL\\Cube_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(md3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mPixelShader.GetAddressOf()));

	return true;
}
bool GameApp::InitResource()
{
	// ******************
	// 设置立方体顶点
	//    5________ 6
	//    /|      /|
	//   /_|_____/ |
	//  1|4|_ _ 2|_|7
	//   | /     | /
	//   |/______|/
	//  0       3
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) }
	};
	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mVertexBuffer.GetAddressOf()));

	// ******************
	// 索引数组
	WORD indices[] = {
		// 正面
		0, 1, 2,
		2, 3, 0,
		// 左面
		4, 5, 1,
		1, 0, 4,
		// 顶面
		1, 5, 6,
		6, 2, 1,
		// 背面
		7, 6, 5,
		5, 4, 7,
		// 右面
		3, 2, 6,
		6, 7, 3,
		// 底面
		4, 0, 3,
		3, 7, 4
	};
	// 设置索引缓冲区描述
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof indices;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = indices;
	HR(md3dDevice->CreateBuffer(&ibd, &InitData, mIndexBuffer.GetAddressOf()));
	// 输入装配阶段的索引缓冲区设置
	md3dImmediateContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);


	// ******************
	// 设置常量缓冲区描述
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;
	// 新建常量缓冲区，不使用初始数据
	HR(md3dDevice->CreateBuffer(&cbd, nullptr, mConstantBuffer.GetAddressOf()));

	// 初始化常量缓冲区的值
	mCBuffer.world = XMMatrixIdentity();
	mCBuffer.view = XMMatrixTranspose(XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f),
		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	));
	mCBuffer.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f));

	// ******************
	// 给渲染管线各个阶段绑定好所需资源

	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexPosColor);	// 跨越字节数
	UINT offset = 0;						// 起始偏移量

	md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
	// 设置图元类型，设定输入布局
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dImmediateContext->IASetInputLayout(mVertexLayout.Get());
	// 将着色器绑定到渲染管线
	md3dImmediateContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
	// 将更新好的常量缓冲区绑定到顶点着色器
	md3dImmediateContext->VSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());

	md3dImmediateContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

	return true;
}
