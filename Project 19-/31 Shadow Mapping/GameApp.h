#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <WinMin.h>
#include "d3dApp.h"
#include "Effects.h"
#include <CameraController.h>
#include <RenderStates.h>
#include <GameObject.h>
#include <Texture2D.h>
#include <Buffer.h>
#include <Collision.h>
#include <ModelManager.h>
#include <TextureManager.h>

class GameApp : public D3DApp
{
public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();
    void DrawScene(ID3D11ShaderResourceView* pShadow, ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV, BasicEffect& effect, const D3D11_VIEWPORT& vp, bool enableNormalMap = false);
    void DrawScene(ID3D11DepthStencilView* pDSV, ShadowEffect& effect, const D3D11_VIEWPORT& vp);

private:
    bool InitResource();

private:

    TextureManager m_TextureManager;
    ModelManager m_ModelManager;

    bool m_UpdateLight = true;                                  // 更新灯光
    bool m_EnableNormalMap = true;								// 开启法线贴图
    bool m_EnableDebug = true;									// 开启调试模式
    int m_SlopeIndex = 0;			  							// 斜率索引
    uint32_t m_MsaaLevels = 4;                                  // MSAA级别

    GameObject m_Ground;										// 地面
    GameObject m_Cylinders[10];								    // 圆柱体
    GameObject m_Spheres[10];									// 球体
    GameObject m_House;											// 房屋
    GameObject m_Skybox;                                        // 天空盒

    std::unique_ptr<Depth2DMS> m_pDepthTexture;                 // 深度纹理
    std::unique_ptr<Texture2DMS> m_pLitTexture;                 // 场景渲染缓冲区
    std::unique_ptr<Depth2D> m_pShadowMapTexture;               // 阴影贴图
    std::unique_ptr<Texture2D> m_pDebugShadowTexture;           // 调试用阴影纹理

    DirectionalLight m_DirLights[3] = {};						// 方向光
    DirectX::XMFLOAT3 m_OriginalLightDirs[3] = {};				// 初始光方向

    BasicEffect m_BasicEffect;				                    // 基础特效
    ShadowEffect m_ShadowEffect;				                // 阴影特效
    SkyboxEffect m_SkyboxEffect;					                // 天空盒特效
    
    std::shared_ptr<FirstPersonCamera> m_pCamera;	            // 摄像机
    FirstPersonCameraController m_CameraController;             // 摄像机控制器
};


#endif