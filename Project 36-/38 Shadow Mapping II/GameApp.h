#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include "d3dApp.h"
#include "CameraController.h"
#include "Effects.h"
#include "RenderStates.h"
#include "GameObject.h"
#include "Collision.h"
#include "Texture2D.h"
#include "Buffer.h"
#include "TextureManager.h"



class GameApp : public D3DApp
{
public:
	// 摄像机模式
	enum class CameraMode { FirstPerson, ThirdPerson, Free };

public:
	GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth = 1280, int initHeight = 720);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

	void RenderForward(bool doPreZ);
	void RenderSkyboxAndToneMap();

private:
	
	// 各种资源
	TextureManager m_TextureManager;                                // 纹理读取管理
	std::unique_ptr<Texture2D> m_pLitBuffer;                        // 场景渲染缓冲区
	std::unique_ptr<Depth2D> m_pDepthBuffer;                        // 深度缓冲区

	// 方向光
	DirectX::XMFLOAT3 m_LightPos;
	float m_LightIntensity;

	// 模型
	GameObject m_Floor;											    // 地面模型
	GameObject m_Marry;                                             // 少女模型
	GameObject m_Skybox;											// 天空盒模型

	// 特效
	std::unique_ptr<ForwardEffect> m_pForwardEffect;				// 前向渲染特效
	std::unique_ptr<SkyboxToneMapEffect> m_pSkyboxEffect;			// 天空盒特效
	ComPtr<ID3D11ShaderResourceView> m_pTextureCubeSRV;				// 天空盒纹理

	// 摄像机
	std::shared_ptr<Camera> m_pCamera;								// 摄像机
	FirstPersonCameraController m_FPSCameraController;				// 摄像机控制器

};


#endif