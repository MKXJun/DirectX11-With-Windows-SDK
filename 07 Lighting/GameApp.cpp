#include "GameApp.h"
#include <filesystem>
using namespace DirectX;
using namespace std::experimental;

const D3D11_INPUT_ELEMENT_DESC GameApp::VertexPosNormalColor::inputLayout[3] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
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

	return true;
}

void GameApp::OnResize()
{
	D3DApp::OnResize();
}

void GameApp::UpdateScene(float dt)
{
	// 更新常量缓冲区，让立方体转起来
	static float phi = 0.0f, theta = 0.0f;
	phi += 0.00003f, theta += 0.00005f;
	mVSConstantBuffer.world = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
	mVSConstantBuffer.worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, mVSConstantBuffer.world));
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[0].Get(), 0, nullptr, &mVSConstantBuffer, 0, 0);

	// 键盘切换灯光类型
	Keyboard::State state = mKeyboard->GetState();
	mKeyboardTracker.Update(state);	
	if (mKeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		mPSConstantBuffer.dirLight = mDirLight;
		mPSConstantBuffer.pointLight = PointLight();
		mPSConstantBuffer.spotLight = SpotLight();
		md3dImmediateContext->UpdateSubresource(mConstantBuffers[1].Get(), 0, nullptr, &mPSConstantBuffer, 0, 0);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		mPSConstantBuffer.dirLight = DirectionalLight();
		mPSConstantBuffer.pointLight = mPointLight;
		mPSConstantBuffer.spotLight = SpotLight();
		md3dImmediateContext->UpdateSubresource(mConstantBuffers[1].Get(), 0, nullptr, &mPSConstantBuffer, 0, 0);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		mPSConstantBuffer.dirLight = DirectionalLight();
		mPSConstantBuffer.pointLight = PointLight();
		mPSConstantBuffer.spotLight = mSpotLight;
		md3dImmediateContext->UpdateSubresource(mConstantBuffers[1].Get(), 0, nullptr, &mPSConstantBuffer, 0, 0);
	}

	// 键盘切换模型类型
	if (mKeyboardTracker.IsKeyPressed(Keyboard::Q))
	{
		Geometry::MeshData meshData = Geometry::CreateBox();
		ResetMesh(meshData);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::W))
	{
		Geometry::MeshData meshData = Geometry::CreateSphere();
		ResetMesh(meshData);
	}
	else if (mKeyboardTracker.IsKeyPressed(Keyboard::E))
	{
		Geometry::MeshData meshData = Geometry::CreateCylinder();
		ResetMesh(meshData);
	}
}

void GameApp::DrawScene()
{
	assert(md3dImmediateContext);
	assert(mSwapChain);

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// VS常量缓冲区对应HLSL寄存于b0的常量缓冲区
	md3dImmediateContext->VSSetConstantBuffers(0, 1, mConstantBuffers[0].GetAddressOf());
	// PS常量缓冲区对应HLSL寄存于b1的常量缓冲区
	md3dImmediateContext->PSSetConstantBuffers(1, 1, mConstantBuffers[1].GetAddressOf());
	
	
	
	// 绘制几何模型
	md3dImmediateContext->DrawIndexed(mIndexCount, 0, 0);

	HR(mSwapChain->Present(0, 0));
}

HRESULT GameApp::CompileShaderFromFile(const WCHAR * szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob ** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// 设置 D3DCOMPILE_DEBUG 标志用于获取着色器调试信息。该标志可以提升调试体验，
	// 但仍然允许着色器进行优化操作
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// 在Debug环境下禁用优化以避免出现一些不合理的情况
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, errorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		if (errorBlob != nullptr)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
		}
		return hr;
	}


	return S_OK;
}



bool GameApp::InitEffect()
{
	ComPtr<ID3DBlob> blob;

	// 已经编译好的着色器文件名
	filesystem::path psoPath = "HLSL\\Light_PS.cso", vsoPath = "HLSL\\Light_VS.cso";
	filesystem::file_status fs;
	std::wstring wstr;
	// 寻找是否有已经编译好的顶点着色器，否则在运行期编译
	if (filesystem::exists(vsoPath))
	{
		wstr = vsoPath.generic_wstring();
		HR(D3DReadFileToBlob(wstr.c_str(), blob.GetAddressOf()));
	}
	else
	{
		HR(CompileShaderFromFile(L"HLSL\\Light.fx", "VS", "vs_5_0", blob.GetAddressOf()));
	}
	// 创建顶点着色器
	HR(md3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mVertexShader.GetAddressOf()));
	// 创建并绑定顶点布局
	HR(md3dDevice->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), mVertexLayout.GetAddressOf()));
	md3dImmediateContext->IASetInputLayout(mVertexLayout.Get());

	blob.Reset();
	

	// 寻找是否有已经编译好的像素着色器，否则在运行期编译
	if (filesystem::exists(psoPath))
	{
		wstr = psoPath.generic_wstring();
		HR(D3DReadFileToBlob(wstr.c_str(), blob.GetAddressOf()));
	}
	else
	{
		HR(CompileShaderFromFile(L"HLSL\\Light.fx", "PS", "ps_5_0", blob.GetAddressOf()));
	}
	// 创建像素着色器
	HR(md3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mPixelShader.GetAddressOf()));

	blob.Reset();

	// 将着色器绑定到渲染管线
	md3dImmediateContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
	md3dImmediateContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

	return true;
}

bool GameApp::InitResource()
{
	// 初始化网格模型
	Geometry::MeshData meshData = Geometry::CreateBox();
	ResetMesh(meshData);


	// ******************
	// 设置常量缓冲区描述
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(VSConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;
	// 新建用于VS和PS的常量缓冲区
	HR(md3dDevice->CreateBuffer(&cbd, nullptr, mConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = sizeof(PSConstantBuffer);
	HR(md3dDevice->CreateBuffer(&cbd, nullptr, mConstantBuffers[1].GetAddressOf()));

	// 初始化默认光照
	// 方向光
	mDirLight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLight.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mDirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLight.Direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	// 点光
	mPointLight.Position = XMFLOAT3(0.0f, 0.0f, -10.0f);
	mPointLight.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mPointLight.Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mPointLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mPointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mPointLight.Range = 25.0f;
	// 聚光灯
	mSpotLight.Position = XMFLOAT3(0.0f, 0.0f, -5.0f);
	mSpotLight.Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	mSpotLight.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mSpotLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mSpotLight.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mSpotLight.Att = XMFLOAT3(1.0f, 0.0f, 0.0f);
	mSpotLight.Spot = 12.0f;
	mSpotLight.Range = 10000.0f;
	// 初始化用于VS的常量缓冲区的值
	mVSConstantBuffer.world = XMMatrixIdentity();			
	mVSConstantBuffer.view = XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f),
		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	);
	mVSConstantBuffer.proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f);
	mVSConstantBuffer.worldInvTranspose = XMMatrixIdentity();
	
	// 初始化用于PS的常量缓冲区的值
	mPSConstantBuffer.material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mPSConstantBuffer.material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mPSConstantBuffer.material.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
	// 使用默认平行光
	mPSConstantBuffer.dirLight = mDirLight;
	// 注意不要忘记设置此处的观察位置，否则高亮部分会有问题
	mPSConstantBuffer.eyePos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);

	// 更新PS常量缓冲区资源
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[1].Get(), 0, nullptr, &mPSConstantBuffer, 0, 0);
	// 设置图元类型
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return true;
}

bool GameApp::ResetMesh(const Geometry::MeshData & meshData)
{
	// 释放旧资源
	mVertexBuffer.Reset();
	mIndexBuffer.Reset();

	size_t vertexSize = meshData.posVec.size();
	std::vector<VertexPosNormalColor> vertices(vertexSize);
	for (size_t i = 0; i < vertexSize; ++i)
	{
		vertices[i].pos = meshData.posVec[i];
		vertices[i].normal = meshData.normalVec[i];
		vertices[i].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = (UINT)vertices.size() * sizeof(VertexPosNormalColor);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();
	HR(md3dDevice->CreateBuffer(&vbd, &InitData, mVertexBuffer.GetAddressOf()));

	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexPosNormalColor);	// 跨越字节数
	UINT offset = 0;							// 起始偏移量

	md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);



	// 设置索引缓冲区描述
	mIndexCount = (int)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(WORD) * mIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = meshData.indexVec.data();
	HR(md3dDevice->CreateBuffer(&ibd, &InitData, mIndexBuffer.GetAddressOf()));
	// 输入装配阶段的索引缓冲区设置
	md3dImmediateContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	return true;
}
