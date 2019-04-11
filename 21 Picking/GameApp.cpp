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
	RenderStates::InitAll(m_pd3dDevice);

	if (!m_BasicEffect.InitAll(m_pd3dDevice))
		return false;

	if (!InitResource())
		return false;

	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	return true;
}

void GameApp::OnResize()
{
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);
	// 释放D2D的相关资源
	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

	// 为D2D创建DXGI表面渲染目标
	ComPtr<IDXGISurface> surface;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = m_pd2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, m_pd2dRenderTarget.GetAddressOf());
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
		HR(m_pd2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			m_pColorBrush.GetAddressOf()));
		HR(m_pdwriteFactory->CreateTextFormat(L"宋体", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
			m_pTextFormat.GetAddressOf()));
	}
	else
	{
		// 报告异常问题
		assert(m_pd2dRenderTarget);
	}

	// 摄像机变更显示
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
	}
}

void GameApp::UpdateScene(float dt)
{

	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	// ******************
	// 记录并更新物体位置和旋转弧度
	//
	static float theta = 0.0f, phi = 0.0f;
	static XMMATRIX Left = XMMatrixTranslation(-5.0f, 0.0f, 0.0f);
	static XMMATRIX Top = XMMatrixTranslation(0.0f, 4.0f, 0.0f);
	static XMMATRIX Right = XMMatrixTranslation(5.0f, 0.0f, 0.0f);
	static XMMATRIX Bottom = XMMatrixTranslation(0.0f, -4.0f, 0.0f);

	theta += dt * 0.5f;
	phi += dt * 0.3f;
	// 更新物体运动
	m_Sphere.SetWorldMatrix(Left);
	m_Cube.SetWorldMatrix(XMMatrixRotationX(-phi) * XMMatrixRotationY(theta) * Top);
	m_Cylinder.SetWorldMatrix(XMMatrixRotationX(phi) * XMMatrixRotationY(theta) * Right);
	m_House.SetWorldMatrix(XMMatrixScaling(0.005f, 0.005f, 0.005f) * XMMatrixRotationY(theta) * Bottom);
	m_Triangle.SetWorldMatrix(XMMatrixRotationY(theta));

	// ******************
	// 拾取检测
	//
	m_PickedObjStr = L"无";
	Ray ray = Ray::ScreenToRay(*m_pCamera, (float)mouseState.x, (float)mouseState.y);
	
	// 三角形顶点变换
	static XMVECTOR V[3];
	for (int i = 0; i < 3; ++i)
	{
		V[i] = XMVector3TransformCoord(XMLoadFloat3(&m_TriangleMesh.vertexVec[i].pos), 
			XMMatrixRotationY(theta));
	}

	bool hitObject = false;
	if (ray.Hit(m_BoundingSphere))
	{
		m_PickedObjStr = L"球体";
		hitObject = true;
	}
	else if (ray.Hit(m_Cube.GetBoundingOrientedBox()))
	{
		m_PickedObjStr = L"立方体";
		hitObject = true;
	}
	else if (ray.Hit(m_Cylinder.GetBoundingOrientedBox()))
	{
		m_PickedObjStr = L"圆柱体";
		hitObject = true;
	}
	else if (ray.Hit(m_House.GetBoundingOrientedBox()))
	{
		m_PickedObjStr = L"房屋";
		hitObject = true;
	}
	else if (ray.Hit(V[0], V[1], V[2]))
	{
		m_PickedObjStr = L"三角形";
		hitObject = true;
	}

	if (hitObject == true && m_MouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
	{
		std::wstring wstr = L"你点击了";
		wstr += m_PickedObjStr + L"!";
		MessageBox(nullptr, wstr.c_str(), L"注意", 0);
	}

	// 重置滚轮值
	m_pMouse->ResetScrollWheelValue();
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	// ******************
	// 绘制Direct3D部分
	//
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 绘制不需要纹理的模型
	m_BasicEffect.SetTextureUsed(false);
	m_Sphere.Draw(m_pd3dImmediateContext, m_BasicEffect);
	m_Cube.Draw(m_pd3dImmediateContext, m_BasicEffect);
	m_Cylinder.Draw(m_pd3dImmediateContext, m_BasicEffect);
	m_Triangle.Draw(m_pd3dImmediateContext, m_BasicEffect);

	// 绘制需要纹理的模型
	m_BasicEffect.SetTextureUsed(true);
	m_House.Draw(m_pd3dImmediateContext, m_BasicEffect);

	// ******************
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"当前拾取物体: " + m_PickedObjStr;

		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// 初始化游戏对象
	//
	
	// 球体(预先设好包围球)
	m_Sphere.SetModel(Model(m_pd3dDevice, Geometry::CreateSphere()));
	m_BoundingSphere.Center = XMFLOAT3(-5.0f, 0.0f, 0.0f);
	m_BoundingSphere.Radius = 1.0f;
	// 立方体
	m_Cube.SetModel(Model(m_pd3dDevice, Geometry::CreateBox()));
	// 圆柱体
	m_Cylinder.SetModel(Model(m_pd3dDevice, Geometry::CreateCylinder()));
	// 房屋
	m_ObjReader.Read(L"Model\\house.mbo", L"Model\\house.obj");
	m_House.SetModel(Model(m_pd3dDevice, m_ObjReader));
	// 三角形(带反面)
	m_TriangleMesh.vertexVec.assign({
		VertexPosNormalTex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2())
		});
	m_TriangleMesh.indexVec.assign({ 0, 1, 2, 3, 4, 5 });
	m_Triangle.SetModel(Model(m_pd3dDevice, m_TriangleMesh));

	// ******************
	// 初始化摄像机
	//
	m_CameraMode = CameraMode::FirstPerson;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	// 初始化并更新观察矩阵、投影矩阵(摄像机将被固定)
	camera->UpdateViewMatrix();
	m_BasicEffect.SetViewMatrix(camera->GetViewXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjXM());
	

	// ******************
	// 初始化不会变化的值
	//

	// 方向光
	DirectionalLight dirLight;
	dirLight.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
	dirLight.direction = XMFLOAT3(-0.707f, -0.707f, 0.707f);
	m_BasicEffect.SetDirLight(0, dirLight);

	// 默认只按对象绘制
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext, BasicEffect::RenderObject);

	return true;
}

