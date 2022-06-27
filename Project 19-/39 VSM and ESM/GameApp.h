#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <WinMin.h>
#include <d3dApp.h>
#include "Effects.h"
#include <CameraController.h>
#include <RenderStates.h>
#include <GameObject.h>
#include <Texture2D.h>
#include <Buffer.h>
#include <Collision.h>
#include <ModelManager.h>
#include <TextureManager.h>
#include "CascadedShadowManager.h"


class GameApp : public D3DApp
{
public:

public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth = 1280, int initHeight = 720);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    bool InitResource();

    void RenderShadowForAllCascades();
    void RenderForward();
    void RenderSkybox();
    


private:
    
    // GPU计时
    GpuTimer m_GpuTimer_Shadow;
    GpuTimer m_GpuTimer_Lighting;
    GpuTimer m_GpuTimer_Skybox;

    // MSAA
    int m_MsaaSamples = 1;
    
    int m_DebugShadowIndex = 1;

    // 阴影
    CascadedShadowManager m_CSManager;
    bool m_DebugShadow = false;

    // 各种资源
    TextureManager m_TextureManager;                                // 纹理读取管理
    ModelManager m_ModelManager;                                    // 模型读取管理
    std::unique_ptr<Texture2DMS> m_pLitBuffer;                      // 场景渲染缓冲区
    std::unique_ptr<Depth2DMS> m_pDepthBuffer;                      // 深度缓冲区
    std::unique_ptr<Texture2D> m_pDebugShadowBuffer;                // 调试用shadow map纹理

    // 模型
    GameObject m_Powerplant;                                        // 发电厂模型
    GameObject m_Cube;                                              // 测试立方体
    GameObject m_Skybox;                                            // 天空盒模型

    // 特效
    ForwardEffect m_ForwardEffect;                                  // 前向渲染特效
    ShadowEffect m_ShadowEffect;                                    // 阴影特效
    SkyboxEffect m_SkyboxEffect;                                    // 天空盒特效

    // 摄像机
    std::shared_ptr<Camera> m_pViewerCamera;                        // 用户摄像机
    std::shared_ptr<Camera> m_pLightCamera;                         // 光源摄像机
    FirstPersonCameraController m_FPSCameraController;              // 摄像机控制器

};


#endif