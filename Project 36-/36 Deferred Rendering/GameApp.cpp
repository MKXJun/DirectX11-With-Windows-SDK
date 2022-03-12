#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

#pragma warning(disable: 26812)

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
	: D3DApp(hInstance, windowName, initWidth, initHeight),
	m_pSkyboxEffect(std::make_unique<SkyboxToneMapEffect>()),
	m_pForwardEffect(std::make_unique<ForwardEffect>()),
	m_pDeferredEffect(std::make_unique<DeferredEffect>())
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

	if (!m_pDeferredEffect->InitAll(m_pd3dDevice.Get(), m_MsaaSamples))
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

	// 摄像机变更显示
	if (m_pCamera != nullptr)
	{
		// 注意：反转Z时需要将近/远平面对调
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 300.0f, 0.5f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_pForwardEffect->SetProjMatrix(m_pCamera->GetProjMatrixXM(true));
		m_pDeferredEffect->SetProjMatrix(m_pCamera->GetProjMatrixXM(true));
		m_pDeferredEffect->SetCameraNearFar(0.5f, 300.0f);
	}

	ResizeBuffers(m_ClientWidth, m_ClientHeight, m_MsaaSamples);
}

void GameApp::UpdateScene(float dt)
{
	// 更新摄像机
	m_FPSCameraController.Update(dt);

	if (ImGui::Begin("Deferred Rendering"))
	{
		static const char* msaa_modes[] = {
			"None",
			"2x MSAA",
			"4x MSAA",
			"8x MSAA"
		};
		static int curr_msaa_item = 0;
		if (ImGui::Combo("MSAA", &curr_msaa_item, msaa_modes, ARRAYSIZE(msaa_modes)))
		{
			switch (curr_msaa_item)
			{
			case 0: m_MsaaSamples = 1; break;
			case 1: m_MsaaSamples = 2; break;
			case 2: m_MsaaSamples = 4; break;
			case 3: m_MsaaSamples = 8; break;
			}
			ResizeBuffers(m_ClientWidth, m_ClientHeight, m_MsaaSamples);
			m_pSkyboxEffect->SetMsaaSamples(m_MsaaSamples);
			m_pDeferredEffect->SetMsaaSamples(m_MsaaSamples);
		}

		static const char* light_culliing_modes[] = {
			"Forward No Culling",
			"Forward Pre-Z No Culling",
			"Deferred No Culling"
		};
		static int curr_light_culliing_item = static_cast<int>(m_LightCullTechnique);
		if (ImGui::Combo("Light Culling", &curr_light_culliing_item, light_culliing_modes, ARRAYSIZE(light_culliing_modes)))
		{
			m_LightCullTechnique = static_cast<LightCullTechnique>(curr_light_culliing_item);
		}

		ImGui::Checkbox("Animate Lights", &m_AnimateLights);
		if (m_AnimateLights)
		{
			UpdateLights(dt);
		}

		if (ImGui::Checkbox("Lighting Only", &m_LightingOnly))
		{
			m_pDeferredEffect->SetLightingOnly(m_LightingOnly);
		}

		if (ImGui::Checkbox("Face Normals", &m_FaceNormals))
		{
			m_pDeferredEffect->SetFaceNormals(m_FaceNormals);
		}

		ImGui::Checkbox("Clear G-Buffers", &m_ClearGBuffers);

		if (ImGui::Checkbox("Visualize Light Count", &m_VisualizeLightCount))
		{
			m_pDeferredEffect->SetVisualizeLightCount(m_VisualizeLightCount);
		}

		if (ImGui::Checkbox("Visualize Shading Freq", &m_VisualizeShadingFreq))
		{
			m_pDeferredEffect->SetVisualizeShadingFreq(m_VisualizeShadingFreq);
		}
		
		ImGui::Text("Light Height Scale");
		ImGui::PushID(0);
		if (ImGui::SliderFloat("", &m_LightHeightScale, 0.25f, 1.0f))
		{
			UpdateLights(0.0f);
		}
		ImGui::PopID();

		static int light_level = static_cast<int>(log2f(static_cast<float>(m_ActiveLights)));
		ImGui::Text("Lights: %d", m_ActiveLights);
		ImGui::PushID(1);
		if (ImGui::SliderInt("", &light_level, 0, 10, ""))
		{
			m_ActiveLights = (1 << light_level);
			ResizeLights(m_ActiveLights);
			UpdateLights(0.0f);
		}
		ImGui::PopID();
	}
	ImGui::End();
	

	m_pForwardEffect->SetViewMatrix(m_pCamera->GetViewMatrixXM());
	m_pDeferredEffect->SetViewMatrix(m_pCamera->GetViewMatrixXM());
	SetupLights(m_pCamera->GetViewMatrixXM());

	//
	// 更新物体与视锥体碰撞
	//
	BoundingFrustum frustum; 
	BoundingFrustum::CreateFromMatrix(frustum, m_pCamera->GetProjMatrixXM());
	frustum.Transform(frustum, m_pCamera->GetLocalToWorldMatrixXM());
	m_Sponza.FrustumCulling(frustum);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	//
	// 场景渲染部分
	//
	if (m_LightCullTechnique == LightCullTechnique::CULL_FORWARD_NONE)
		RenderForward(false);
	else if (m_LightCullTechnique == LightCullTechnique::CULL_FORWARD_PREZ_NONE)
		RenderForward(true);
	else if (m_LightCullTechnique == LightCullTechnique::CULL_DEFERRED_NONE){
		RenderGBuffer();
		m_pDeferredEffect->ComputeLightingDefault(m_pd3dImmediateContext.Get(), m_pLitBuffer->GetRenderTarget(),
			m_pDepthBufferReadOnlyDSV.Get(), m_pLightBuffer->GetShaderResource(),
			m_pGBufferSRVs.data(), m_pCamera->GetViewPort());
	}
	RenderSkyboxAndToneMap();

	//
	// ImGui部分
	//
	if (ImGui::Begin("Normal"))
	{
		m_pDeferredEffect->DebugNormalGBuffer(m_pd3dImmediateContext.Get(), m_pDebugNormalGBuffer->GetRenderTarget(), 
			m_pGBufferSRVs[0], m_pCamera->GetViewPort());
		ImVec2 winSize = ImGui::GetWindowSize();
		float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
		ImGui::Image(m_pDebugNormalGBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
	}
	ImGui::End();

	if (ImGui::Begin("Albedo"))
	{
		m_pd3dImmediateContext->ResolveSubresource(m_pDebugAlbedoGBuffer->GetTexture(), 0, m_pGBuffers[1]->GetTexture(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		ImVec2 winSize = ImGui::GetWindowSize();
		float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
		ImGui::Image(m_pDebugAlbedoGBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
	}
	ImGui::End();

	if (ImGui::Begin("PosZGrad"))
	{
		m_pd3dImmediateContext->ResolveSubresource(m_pDebugPosZGradGBuffer->GetTexture(), 0, m_pGBuffers[2]->GetTexture(), 0, DXGI_FORMAT_R16G16_FLOAT);
		ImVec2 winSize = ImGui::GetWindowSize();
		float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
		ImGui::Image(m_pDebugPosZGradGBuffer->GetShaderResource(), ImVec2(smaller * AspectRatio(), smaller));
	}
	ImGui::End();

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
	camera->LookAt(XMFLOAT3(-60.0f, 10.0f, -2.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

	m_FPSCameraController.InitCamera(camera.get());
	m_FPSCameraController.SetMoveSpeed(10.0f);
	m_FPSCameraController.SetStrafeSpeed(10.0f);
	// ******************
	// 初始化特效
	//

	m_pForwardEffect->SetViewMatrix(camera->GetViewMatrixXM());
	m_pDeferredEffect->SetViewMatrix(camera->GetViewMatrixXM());
	// 注意：反转Z
	m_pForwardEffect->SetProjMatrix(camera->GetProjMatrixXM(true));
	m_pDeferredEffect->SetProjMatrix(camera->GetProjMatrixXM(true));
	m_pDeferredEffect->SetCameraNearFar(0.5f, 300.0f);
	m_pDeferredEffect->SetMsaaSamples(1);
	m_pSkyboxEffect->SetMsaaSamples(1);

	// ******************
	// 初始化对象
	//
	Model sponza;
	sponza.SetModel(m_pd3dDevice.Get(), "..\\Model\\Sponza\\sponza.obj");
	m_Sponza.SetModel(std::move(sponza));
	m_Sponza.GetTransform().SetScale(0.05f, 0.05f, 0.05f);

	m_Skybox.SetModel(Model(m_pd3dDevice.Get(), Geometry::CreateBox()));


	// ******************
	// 初始化天空盒纹理
	//
	m_pTextureCubeSRV = m_TextureManager.CreateTexture(L"..\\Texture\\Clouds.dds");

	// ******************
	// 初始化光照
	//

	InitLightParams();
	ResizeLights(m_ActiveLights);
	UpdateLights(0.0f);

	// ******************
	// 设置调试对象名
	//
	m_Sponza.SetDebugObjectName("Sponza");
	m_Skybox.SetDebugObjectName("Skybox");

	return true;
}

void GameApp::InitLightParams()
{
	m_PointLightParams.resize(MAX_LIGHTS);
	m_PointLightInitDatas.resize(MAX_LIGHTS);
	m_PointLightPosWorlds.resize(MAX_LIGHTS);

	// 使用固定的随机数种子保持一致性
	std::mt19937 rng(1337);
	constexpr float maxRadius = 100.0f;
	constexpr float attenuationStartFactor = 0.8f;
	std::uniform_real<float> radiusNormDist(0.0f, 1.0f);
	std::uniform_real<float> angleDist(0.0f, 2.0f * XM_PI);
	std::uniform_real<float> heightDist(0.0f, 75.0f);
	std::uniform_real<float> animationSpeedDist(2.0f, 20.0f);
	std::uniform_int<int> animationDirection(0, 1);
	std::uniform_real<float> hueDist(0.0f, 1.0f);
	std::uniform_real<float> intensityDist(0.2f, 0.8f);
	std::uniform_real<float> attenuationDist(2.0f, 20.0f);

	for (unsigned int i = 0; i < MAX_LIGHTS; ++i) {
		PointLight& params = m_PointLightParams[i];
		PointLightInitData& data = m_PointLightInitDatas[i];

		data.radius = std::sqrt(radiusNormDist(rng)) * maxRadius;
		data.angle = angleDist(rng);
		data.height = heightDist(rng);
		// 归一化速度
		data.animationSpeed = (animationDirection(rng) * 2 - 1) * animationSpeedDist(rng) / data.radius;

		// HSL->RGB
		params.color = HueToRGB(hueDist(rng));
		XMStoreFloat3(&params.color, XMLoadFloat3(&params.color) * intensityDist(rng));
		params.attenuationEnd = attenuationDist(rng);
		params.attenuationBegin = attenuationStartFactor * params.attenuationEnd;
	}
}

XMFLOAT3 GameApp::HueToRGB(float hue)
{
	float intPart;
	float fracPart = std::modf(hue * 6.0f, &intPart);
	int region = static_cast<int>(intPart);

	switch (region)
	{
	case 0: return XMFLOAT3(1.0f, fracPart, 0.0f);
	case 1: return XMFLOAT3(1.0f - fracPart, 1.0f, 0.0f);
	case 2: return XMFLOAT3(0.0f, 1.0f, fracPart);
	case 3: return XMFLOAT3(0.0f, 1.0f - fracPart, 1.0f);
	case 4: return XMFLOAT3(fracPart, 0.0f, 1.0f);
	case 5: return XMFLOAT3(1.0f, 0.0f, 1.0f - fracPart);
	}
	return DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void GameApp::ResizeLights(UINT activeLights)
{
	m_ActiveLights = activeLights;
	m_pLightBuffer = std::make_unique<StructuredBuffer<PointLight>>(m_pd3dDevice.Get(), activeLights, D3D11_BIND_SHADER_RESOURCE, true);

	// ******************
	// 设置调试对象名
	//
	m_pLightBuffer->SetDebugObjectName("LightBuffer");
}

void GameApp::UpdateLights(float dt)
{
	static float totalTime = 0.0f;
	totalTime += dt;
	for (UINT i = 0; i < m_ActiveLights; ++i)
	{
		const auto& data = m_PointLightInitDatas[i];
		float angle = data.angle + totalTime * data.animationSpeed;
		m_PointLightPosWorlds[i] = XMFLOAT3(
			data.radius * std::cos(angle),
			data.height * m_LightHeightScale,
			data.radius * std::sin(angle)
		);
	}
}

void XM_CALLCONV GameApp::SetupLights(DirectX::XMMATRIX viewMatrix)
{
	XMVector3TransformCoordStream(&m_PointLightParams[0].posV, sizeof(PointLight),
		&m_PointLightPosWorlds[0], sizeof(XMFLOAT3), m_ActiveLights, viewMatrix);

	PointLight* pData = m_pLightBuffer->MapDiscard(m_pd3dImmediateContext.Get());
	memcpy_s(pData, sizeof(PointLight) * m_ActiveLights,
		m_PointLightParams.data(), sizeof(PointLight) * m_ActiveLights);
	m_pLightBuffer->Unmap(m_pd3dImmediateContext.Get());
}

void GameApp::ResizeBuffers(UINT width, UINT height, UINT msaaSamples)
{
	// ******************
	// 初始化延迟渲染所需资源
	//
	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = msaaSamples;
	sampleDesc.Quality = 0;
	m_pLitBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height,
		DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		sampleDesc);
	m_pDepthBuffer = std::make_unique<Depth2D>(m_pd3dDevice.Get(), width, height,
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE, sampleDesc,
		msaaSamples > 1);	// 使用MSAA则需要提供模板

	// 创建只读深度/模板视图
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		m_pDepthBuffer->GetDepthStencil()->GetDesc(&desc);
		desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
		m_pd3dDevice->CreateDepthStencilView(m_pDepthBuffer->GetTexture(), &desc, m_pDepthBufferReadOnlyDSV.ReleaseAndGetAddressOf());
	}

	// G-Buffer
	// MRT要求所有的G-Buffer使用相同的MSAA采样等级
	m_pGBuffers.clear();
	// normal_specular
	m_pGBuffers.push_back(std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		sampleDesc));
	// albedo
	m_pGBuffers.push_back(std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		sampleDesc));
	// posZgrad
	m_pGBuffers.push_back(std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R16G16_FLOAT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		sampleDesc));

	// 设置GBuffer资源列表
	m_pGBufferRTVs.resize(m_pGBuffers.size(), 0);
	m_pGBufferSRVs.resize(4, 0);
	for (std::size_t i = 0; i < m_pGBuffers.size(); ++i) {
		m_pGBufferRTVs[i] = m_pGBuffers[i]->GetRenderTarget();
		m_pGBufferSRVs[i] = m_pGBuffers[i]->GetShaderResource();
	}
	// 深度缓冲区作为最后的SRV用于读取
	m_pGBufferSRVs.back() = m_pDepthBuffer->GetShaderResource();

	// 调试用缓冲区
	m_pDebugNormalGBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
	m_pDebugAlbedoGBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
	m_pDebugPosZGradGBuffer = std::make_unique<Texture2D>(m_pd3dDevice.Get(), width, height, DXGI_FORMAT_R16G16_FLOAT);
	// ******************
	// 设置调试对象名
	//
	m_pDepthBuffer->SetDebugObjectName("DepthBuffer");
	D3D11SetDebugObjectName(m_pDepthBufferReadOnlyDSV.Get(), "DepthBufferReadOnlyDSV");
	m_pLitBuffer->SetDebugObjectName("LitBuffer");
	m_pGBuffers[0]->SetDebugObjectName("GBuffer_Normal_Specular");
	m_pGBuffers[1]->SetDebugObjectName("GBuffer_Albedo");
	m_pGBuffers[2]->SetDebugObjectName("GBuffer_PosZgrad");
}

void GameApp::RenderForward(bool doPreZ)
{
	float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pd3dImmediateContext->ClearRenderTargetView(m_pLitBuffer->GetRenderTarget(), black);
	// 注意：反向Z的缓冲区，远平面为0
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthBuffer->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

	D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
	m_pd3dImmediateContext->RSSetViewports(1, &viewport);

	// PreZ Pass
	if (doPreZ)
	{
		m_pd3dImmediateContext->OMSetRenderTargets(0, 0, m_pDepthBuffer->GetDepthStencil());
		m_pForwardEffect->SetRenderPreZPass(m_pd3dImmediateContext.Get());
		m_Sponza.Draw(m_pd3dImmediateContext.Get(), m_pForwardEffect.get());
	}

	// 正常绘制
	ID3D11RenderTargetView* pRTVs[1] = { m_pLitBuffer->GetRenderTarget() };
	m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthBuffer->GetDepthStencil());

	m_pForwardEffect->SetRenderDefault(m_pd3dImmediateContext.Get());
	m_pForwardEffect->SetLightBuffer(m_pLightBuffer->GetShaderResource());
	m_Sponza.Draw(m_pd3dImmediateContext.Get(), m_pForwardEffect.get());

	// 清除绑定
	m_pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void GameApp::RenderGBuffer()
{
	// 注意：我们实际上只需要清空深度缓冲区，因为我们用天空盒的采样来替代没有写入的像素(例如：远平面)
	// 	    这里我们使用深度缓冲区来重建位置且只有在视锥体内的位置会被着色
	// 注意：反转Z的缓冲区，远平面为0
	if (m_ClearGBuffers)
	{
		float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		for (auto rtv : m_pGBufferRTVs)
			m_pd3dImmediateContext->ClearRenderTargetView(rtv, black);
	}
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthBuffer->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);
	
	D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
	m_pd3dImmediateContext->RSSetViewports(1, &viewport);

	m_pDeferredEffect->SetRenderGBuffer(m_pd3dImmediateContext.Get());
	m_pd3dImmediateContext->OMSetRenderTargets(static_cast<UINT>(m_pGBuffers.size()), m_pGBufferRTVs.data(), m_pDepthBuffer->GetDepthStencil());
	m_Sponza.Draw(m_pd3dImmediateContext.Get(), m_pDeferredEffect.get());

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

