#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

#pragma warning(disable: 26812)

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
	: D3DApp(hInstance, windowName, initWidth, initHeight),
	m_pSkyboxEffect(std::make_unique<SkyboxToneMapEffect>()),
	m_pForwardEffect(std::make_unique<ForwardEffect>())
{
}

GameApp::~GameApp()
{
	/*ComPtr<ID3D11Debug> pDebug;
	m_pd3dDevice.As(&pDebug);
	pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);*/
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	m_TextureManager.Init(m_pd3dDevice.Get());

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_pForwardEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_pSkyboxEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

	return true;
}

void GameApp::OnResize()
{
	D3DApp::OnResize();
	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;
	m_pLitBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, sampleDesc);
	m_pDepthBuffer = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, 
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE, sampleDesc);


	// 摄像机变更显示
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 300.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_pForwardEffect->SetProjMatrix(m_pCamera->GetProjMatrixXM(true));
	}
}

void GameApp::UpdateScene(float dt)
{
	// 更新摄像机
	m_FPSCameraController.Update(dt);

	m_pForwardEffect->SetViewMatrix(m_pCamera->GetViewMatrixXM());
	
	// 旋转灯光
	XMStoreFloat3(&m_LightPos, XMVector3TransformCoord(XMLoadFloat3(&m_LightPos), XMMatrixRotationY(dt * 0.35f)));

	//
	// 更新物体与视锥体碰撞
	//
	BoundingFrustum frustum; 
	BoundingFrustum::CreateFromMatrix(frustum, m_pCamera->GetProjMatrixXM());
	frustum.Transform(frustum, m_pCamera->GetLocalToWorldMatrixXM());
	m_Marry.FrustumCulling(frustum);
	m_Floor.FrustumCulling(frustum);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	//
	// 场景渲染部分
	//
	RenderForward(true);
	RenderSkyboxAndToneMap();

	//
	// ImGui部分
	//
	
	
	ImGui::Render();

	m_pd3dImmediateContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	HR(m_pSwapChain->Present(0, 0));
}

bool GameApp::InitResource()
{
	// ******************
	// 初始化摄像机和控制器
	//

	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;

	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 300.0f);
	camera->LookAt(XMFLOAT3(0.0f, 10.0f, -10.0f), XMFLOAT3(0.0f, 5.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

	m_FPSCameraController.InitCamera(camera.get());
	m_FPSCameraController.SetMoveSpeed(10.0f);
	m_FPSCameraController.SetStrafeSpeed(10.0f);
	// ******************
	// 初始化特效
	//

	m_pForwardEffect->SetViewMatrix(camera->GetViewMatrixXM());
	// 注意：反转Z
	m_pForwardEffect->SetProjMatrix(camera->GetProjMatrixXM(true));
	m_pSkyboxEffect->SetMsaaSamples(1);

	// ******************
	// 初始化对象
	//
	m_Marry.LoadModelFromFile(m_pd3dDevice.Get(), "..\\Model\\marry\\marry.obj");
	m_Marry.GetTransform().SetScale(3.0f, 3.0f, 3.0f);
	m_Floor.LoadModelFromFile(m_pd3dDevice.Get(), "..\\Model\\Floor\\floor.obj");
	m_Skybox.LoadModelFromGeometry(m_pd3dDevice.Get(), Geometry::CreateBox());

	// ******************
	// 初始化天空盒纹理
	//
	m_pTextureCubeSRV = m_TextureManager.CreateTexture("..\\Texture\\Clouds.dds");

	// ******************
	// 初始化光照
	//
	m_LightPos = DirectX::XMFLOAT3(0.0f, 80.0f, 80.0f);
	m_LightIntensity = 5000.0f;

	// ******************
	// 设置调试对象名
	//
	m_Marry.SetDebugObjectName("Marry");
	m_Floor.SetDebugObjectName("Floor");
	m_Skybox.SetDebugObjectName("Skybox");

	return true;
}

void GameApp::RenderForward(bool doPreZ)
{
	float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pd3dImmediateContext->ClearRenderTargetView(m_pLitBuffer->GetRenderTarget(), black);
	// 注意：反向Z的缓冲区，远平面为0
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthBuffer->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

	D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
	m_pd3dImmediateContext->RSSetViewports(1, &viewport);

	// 正常绘制
	ID3D11RenderTargetView* pRTVs[1] = { m_pLitBuffer->GetRenderTarget() };
	m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthBuffer->GetDepthStencil());

	m_pForwardEffect->SetRenderDefault(m_pd3dImmediateContext.Get());
	m_pForwardEffect->SetLightPos(m_LightPos);
	m_pForwardEffect->SetLightIntensity(m_LightIntensity);
	
	m_Floor.Draw(m_pd3dImmediateContext.Get(), m_pForwardEffect.get());
	m_Marry.Draw(m_pd3dImmediateContext.Get(), m_pForwardEffect.get());

	// 清除绑定
	m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
}


void GameApp::RenderSkyboxAndToneMap()
{
	D3D11_VIEWPORT skyboxViewport = m_pCamera->GetViewPort();
	skyboxViewport.MinDepth = 1.0f;
	skyboxViewport.MaxDepth = 1.0f;
	m_pd3dImmediateContext->RSSetViewports(1, &skyboxViewport);

	m_pSkyboxEffect->SetRenderDefault(m_pd3dImmediateContext.Get());

	m_pSkyboxEffect->SetViewMatrix(m_pCamera->GetViewMatrixXM());
	// 注意：反转Z
	m_pSkyboxEffect->SetProjMatrix(m_pCamera->GetProjMatrixXM(true));

	m_pSkyboxEffect->SetSkyboxTexture(m_pTextureCubeSRV.Get());
	m_pSkyboxEffect->SetLitTexture(m_pLitBuffer->GetShaderResource());
	m_pSkyboxEffect->SetDepthTexture(m_pDepthBuffer->GetShaderResource());
	
	// 由于全屏绘制，不需要用到深度缓冲区，也就不需要清空后备缓冲区了
	m_pd3dImmediateContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);
	m_Skybox.Draw(m_pd3dImmediateContext.Get(), m_pSkyboxEffect.get());

	// 清除状态
	m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_pSkyboxEffect->SetLitTexture(nullptr);
	m_pSkyboxEffect->SetDepthTexture(nullptr);
	m_pSkyboxEffect->Apply(m_pd3dImmediateContext.Get());
}

