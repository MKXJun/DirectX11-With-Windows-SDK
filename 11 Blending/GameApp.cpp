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

	if (!InitEffect())
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
		mCBChangesOnReSize.proj = XMMatrixTranspose(mCamera->GetProjXM());
		md3dImmediateContext->UpdateSubresource(mConstantBuffers[2].Get(), 0, nullptr, &mCBChangesOnReSize, 0, 0);
		md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers[2].GetAddressOf());
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

	// ******************
	// 第三人称摄像机的操作
	//

	// 绕原点旋转
	cam3rd->RotateX(mouseState.y * dt * 1.25f);
	cam3rd->RotateY(mouseState.x * dt * 1.25f);
	cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);

	// 更新观察矩阵，并更新每帧缓冲区
	mCamera->UpdateViewMatrix();
	mCBFrame.eyePos = mCamera->GetPositionXM();
	mCBFrame.view = XMMatrixTranspose(mCamera->GetViewXM());
	

	// 重置滚轮值
	mMouse->ResetScrollWheelValue();
	
	
	// 退出程序，这里应向窗口发送销毁信息
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
	
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[1].Get(), 0, nullptr, &mCBFrame, 0, 0);
}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ********************
	// 1. 绘制不透明对象
	//
	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

	for (auto& wall : mWalls)
		wall.Draw(md3dImmediateContext);
	mFloor.Draw(md3dImmediateContext);

	// ********************
	// 2. 绘制透明对象
	//
	md3dImmediateContext->RSSetState(RenderStates::RSNoCull.Get());
	md3dImmediateContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);

	// 篱笆盒稍微抬起一点高度
	mWireFence.SetWorldMatrix(XMMatrixTranslation(2.0f, 0.01f, 0.0f));
	mWireFence.Draw(md3dImmediateContext);
	mWireFence.SetWorldMatrix(XMMatrixTranslation(-2.0f, 0.01f, 0.0f));
	mWireFence.Draw(md3dImmediateContext);
	// 绘制了篱笆盒后再绘制水面
	mWater.Draw(md3dImmediateContext);
	


	// ********************
	// 绘制Direct2D部分
	//
	if (md2dRenderTarget != nullptr)
	{
		md2dRenderTarget->BeginDraw();
		std::wstring text = L"当前摄像机模式：第三人称视角  Esc退出\n"
			"鼠标移动控制视野 滚轮控制第三人称观察距离";
		md2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), mTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, mColorBrush.Get());
		HR(md2dRenderTarget->EndDraw());
	}

	HR(mSwapChain->Present(0, 0));
}



bool GameApp::InitEffect()
{
	ComPtr<ID3DBlob> blob;

	// 创建顶点着色器(2D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_2D.cso", L"HLSL\\Basic_VS_2D.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(md3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mVertexShader2D.GetAddressOf()));
	// 创建顶点布局(2D)
	HR(md3dDevice->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), mVertexLayout2D.GetAddressOf()));

	// 创建像素着色器(2D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_2D.cso", L"HLSL\\Basic_PS_2D.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(md3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mPixelShader2D.GetAddressOf()));

	// 创建顶点着色器(3D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_3D.cso", L"HLSL\\Basic_VS_3D.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(md3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mVertexShader3D.GetAddressOf()));
	// 创建顶点布局(3D)
	HR(md3dDevice->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), mVertexLayout3D.GetAddressOf()));

	// 创建像素着色器(3D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_3D.cso", L"HLSL\\Basic_PS_3D.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(md3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mPixelShader3D.GetAddressOf()));

	return true;
}

bool GameApp::InitResource()
{
	
	// ******************
	// 设置常量缓冲区描述
	//
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;
	// 新建用于VS和PS的常量缓冲区
	cbd.ByteWidth = sizeof(CBChangesEveryDrawing);
	HR(md3dDevice->CreateBuffer(&cbd, nullptr, mConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesEveryFrame);
	HR(md3dDevice->CreateBuffer(&cbd, nullptr, mConstantBuffers[1].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesOnResize);
	HR(md3dDevice->CreateBuffer(&cbd, nullptr, mConstantBuffers[2].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesRarely);
	HR(md3dDevice->CreateBuffer(&cbd, nullptr, mConstantBuffers[3].GetAddressOf()));
	// ******************
	// 初始化游戏对象
	//
	ComPtr<ID3D11ShaderResourceView> texture;
	Material material;
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	// 初始化篱笆盒
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\WireFence.dds", nullptr, texture.GetAddressOf()));
	mWireFence.SetBuffer(md3dDevice, Geometry::CreateBox());
	mWireFence.SetTexture(texture);
	mWireFence.SetMaterial(material);
	
	// 初始化地板
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\floor.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	mFloor.SetBuffer(md3dDevice, 
		Geometry::CreatePlane(XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
	mFloor.SetTexture(texture);
	mFloor.SetMaterial(material);

	// 初始化墙体
	mWalls.resize(4);
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\brick.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	// 这里控制墙体四个面的生成
	for (int i = 0; i < 4; ++i)
	{
		mWalls[i].SetBuffer(md3dDevice,
			Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 1.5f)));
		XMMATRIX world = XMMatrixRotationX(-XM_PIDIV2) * XMMatrixRotationY(XM_PIDIV2 * i)
			* XMMatrixTranslation(i % 2 ? -10.0f * (i - 2) : 0.0f, 3.0f, i % 2 == 0 ? -10.0f * (i - 1) : 0.0f);
		mWalls[i].SetMaterial(material);
		mWalls[i].SetWorldMatrix(world);
		mWalls[i].SetTexture(texture);
	}
		
	// 初始化水
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"Texture\\water.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	mWater.SetBuffer(md3dDevice,
		Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 20.0f), XMFLOAT2(10.0f, 10.0f)));
	mWater.SetTexture(texture);
	mWater.SetMaterial(material);

	// 初始化采样器状态
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(md3dDevice->CreateSamplerState(&sampDesc, mSamplerState.GetAddressOf()));

	
	// ******************
	// 初始化常量缓冲区的值
	//

	// 初始化每帧可能会变化的值
	mCameraMode = CameraMode::ThirdPerson;
	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	mCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	camera->SetTarget(XMFLOAT3(0.0f, 0.5f, 0.0f));
	camera->SetDistance(5.0f);
	camera->SetDistanceMinMax(2.0f, 14.0f);

	// 初始化仅在窗口大小变动时修改的值
	mCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
	mCBChangesOnReSize.proj = XMMatrixTranspose(mCamera->GetProjXM());

	// ********************
	// 初始化不会变化的值
	//

	// 环境光
	mCBRarely.dirLight[0].Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCBRarely.dirLight[0].Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mCBRarely.dirLight[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCBRarely.dirLight[0].Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	// 灯光
	mCBRarely.pointLight[0].Position = XMFLOAT3(0.0f, 15.0f, 0.0f);
	mCBRarely.pointLight[0].Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCBRarely.pointLight[0].Diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	mCBRarely.pointLight[0].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCBRarely.pointLight[0].Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mCBRarely.pointLight[0].Range = 25.0f;
	mCBRarely.numDirLight = 1;
	mCBRarely.numPointLight = 1;
	mCBRarely.numSpotLight = 0;


	// 更新不容易被修改的常量缓冲区资源
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[2].Get(), 0, nullptr, &mCBChangesOnReSize, 0, 0);
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[3].Get(), 0, nullptr, &mCBRarely, 0, 0);

	// 初始化所有渲染状态
	RenderStates::InitAll(md3dDevice);
	
	
	// ******************
	// 给渲染管线各个阶段绑定好所需资源
	//

	// 设置图元类型，设定输入布局
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dImmediateContext->IASetInputLayout(mVertexLayout3D.Get());
	// 预先绑定各自所需的缓冲区，其中每帧更新的缓冲区需要绑定到两个缓冲区上
	md3dImmediateContext->VSSetConstantBuffers(0, 1, mConstantBuffers[0].GetAddressOf());
	md3dImmediateContext->VSSetConstantBuffers(1, 1, mConstantBuffers[1].GetAddressOf());
	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers[2].GetAddressOf());
	// 默认绑定3D着色器
	md3dImmediateContext->VSSetShader(mVertexShader3D.Get(), nullptr, 0);

	md3dImmediateContext->RSSetState(RenderStates::RSNoCull.Get());

	md3dImmediateContext->PSSetConstantBuffers(0, 1, mConstantBuffers[0].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(1, 1, mConstantBuffers[1].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(3, 1, mConstantBuffers[3].GetAddressOf());
	md3dImmediateContext->PSSetShader(mPixelShader3D.Get(), nullptr, 0);
	md3dImmediateContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

	md3dImmediateContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);

	return true;
}

GameApp::GameObject::GameObject()
	: mWorldMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f)
{
}

DirectX::XMFLOAT3 GameApp::GameObject::GetPosition() const
{
	return XMFLOAT3(mWorldMatrix(3, 0), mWorldMatrix(3, 1), mWorldMatrix(3, 2));
}

template<class VertexType, class IndexType>
void GameApp::GameObject::SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{
	// 释放旧资源
	mVertexBuffer.Reset();
	mIndexBuffer.Reset();

	// 设置顶点缓冲区描述
	mVertexStride = sizeof(VertexType);
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * mVertexStride;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	HR(device->CreateBuffer(&vbd, &InitData, mVertexBuffer.GetAddressOf()));


	// 设置索引缓冲区描述
	mIndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = mIndexCount * sizeof(IndexType);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = meshData.indexVec.data();
	HR(device->CreateBuffer(&ibd, &InitData, mIndexBuffer.GetAddressOf()));



}

void GameApp::GameObject::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	mTexture = texture;
}

void GameApp::GameObject::SetMaterial(const Material & material)
{
	mMaterial = material;
}

void GameApp::GameObject::SetWorldMatrix(const XMFLOAT4X4 & world)
{
	mWorldMatrix = world;
}

void XM_CALLCONV GameApp::GameObject::SetWorldMatrix(FXMMATRIX world)
{
	XMStoreFloat4x4(&mWorldMatrix, world);
}

void GameApp::GameObject::Draw(ComPtr<ID3D11DeviceContext> deviceContext)
{
	// 设置顶点/索引缓冲区
	UINT strides = mVertexStride;
	UINT offsets = 0;
	deviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &strides, &offsets);
	deviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// 获取之前已经绑定到渲染管线上的常量缓冲区并进行修改
	ComPtr<ID3D11Buffer> cBuffer = nullptr;
	deviceContext->VSGetConstantBuffers(0, 1, cBuffer.GetAddressOf());
	XMMATRIX W = XMLoadFloat4x4(&mWorldMatrix);
	CBChangesEveryDrawing cbDrawing;
	cbDrawing.world = XMMatrixTranspose(W);
	cbDrawing.worldInvTranspose = XMMatrixInverse(nullptr, W);
	cbDrawing.material = mMaterial;
	deviceContext->UpdateSubresource(cBuffer.Get(), 0, nullptr, &cbDrawing, 0, 0);
	// 设置纹理
	deviceContext->PSSetShaderResources(0, 1, mTexture.GetAddressOf());
	// 可以开始绘制
	deviceContext->DrawIndexed(mIndexCount, 0, 0);
}
