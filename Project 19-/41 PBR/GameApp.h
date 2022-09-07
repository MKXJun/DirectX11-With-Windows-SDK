#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <WinMin.h>
#include <d3dApp.h>
#include <CameraController.h>
#include <RenderStates.h>
#include <GameObject.h>
#include <Texture2D.h>
#include <Buffer.h>
#include <TextureManager.h>
#include <ModelManager.h>
#include "Effects.h"
#include "CascadedShadowManager.h"

// 编程捕获帧(需支持DirectX 11.2 API)
#if defined(DEBUG) | defined(_DEBUG) && defined(GRAPHICS_DEBUG)
#include <DXGItype.h>  
#include <dxgi1_2.h>  
#include <dxgi1_3.h>  
#include <DXProgrammableCapture.h>  
#endif

struct PointLight
{
    DirectX::XMFLOAT3 pos;
    float range;
    DirectX::XMFLOAT3 color;
    float intensity;
};

class GameApp : public D3DApp
{

public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth = 1280, int initHeight = 720);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    
    bool InitResource();

    void UpdateImGui(float dt);
    void DrawImGui();

    void RenderShadowForAllCascades();
    void RenderGBuffer();
    void RenderSkybox();
    void PostProcess();


private:
    
    // GPU计时
    GpuTimer m_GpuTimer_Shadow;
    GpuTimer m_GpuTimer_Lighting;
    GpuTimer m_GpuTimer_Skybox;
    GpuTimer m_GpuTimer_PostProcess;
    GpuTimer m_GpuTimer_Geometry;

    // 阴影
    CascadedShadowManager m_CSManager;

    // 各种资源
    TextureManager m_TextureManager;                                        // 纹理读取管理
    ModelManager m_ModelManager;                                            // 模型读取管理
    std::unique_ptr<Texture2D> m_pLitBuffer;                                // 场景渲染缓冲区
    ComPtr<ID3D11DepthStencilView> m_pDepthBufferReadOnlyDSV;               // 只读深度模板视图
    std::vector<std::unique_ptr<Texture2D>> m_pGBuffers;                    // G-Buffers
    std::unique_ptr<Texture2D> m_pTempBuffer;                               // 存放天空盒渲染结果的缓冲区
    std::unique_ptr<Depth2D> m_pDepthBuffer;                                // 深度缓冲区
    std::unique_ptr<Texture2D> m_pDebugNormalBuffer;                        // 调试用normal纹理
    std::unique_ptr<Texture2D> m_pDebugAlbedoBuffer;                        // 调试用albedo纹理
    std::unique_ptr<Texture2D> m_pDebugMetallicBuffer;                      // 调试用metallic纹理
    std::unique_ptr<Texture2D> m_pDebugRoughnessBuffer;                     // 调试用roughness纹理


    // 用于一次性传递
    std::vector<ID3D11RenderTargetView*> m_pGBufferRTVs;
    std::vector<ID3D11ShaderResourceView*> m_pGBufferSRVs;

    // 模型
    GameObject m_Sponza;                                                        // Sponza模型
    GameObject m_Sphere;                                                        // 球模型
    std::vector<DirectX::XMFLOAT3> m_SpherePositions;                           // 球的位置
    DirectX::XMFLOAT4 m_SphereAlbedo = {0.8f, 0.8f, 0.8f, 1.0f};                // 球的颜色
    float m_SphereRoughness = 0.5f;                                             // 球的粗糙度
    float m_SphereMetallic = 0.0f;                                              // 球的金属度

    // 特效
    DeferredEffect m_DeferredEffect;                                // 延迟渲染特效
    ShadowEffect m_ShadowEffect;                                    // 阴影特效
    SkyboxToneMapEffect m_SkyboxToneMapEffect;                      // 天空盒特效
    FXAAEffect m_FXAAEffect;                                        // FXAA特效

    // 灯光
    DirectX::XMFLOAT3 m_DirLightColor{};                            // 方向光颜色
    float m_DirLightIntensity{};                                    // 方向光强度

    int m_NumPointLights = 0;
    std::unique_ptr<StructuredBuffer<PointLight>> m_pPointLightBuffer;  // 点光源缓冲区
    std::vector<PointLight> m_PointLights;                              // 点光源信息
    std::vector<PointLight> m_UploadPointLights;                        // 用于上传的点光源信息

    // ImGui
    bool m_NeedGpuTimerReset = false;
    bool m_EnableNormalMap = true;                                      // 使用法线贴图
    bool m_UseTangent = true;                                           // 场景所有物体添加切向量
    bool m_UseTexture = true;                                           // 给球使用纹理
    bool m_DebugNormal = true;                                          // 调试法线
    bool m_DebugAlbedo = true;                                          // 调试Albedo
    bool m_DebugMetallic = true;                                        // 调试Metallic
    bool m_DebugRoughness = true;                                       // 调试Roughness
    SkyboxToneMapEffect::ToneMapping m_ToneMapping = SkyboxToneMapEffect::ToneMapping_Standard;

    // 摄像机
    std::shared_ptr<FirstPersonCamera> m_pViewerCamera;             // 用户摄像机
    std::shared_ptr<FirstPersonCamera> m_pLightCamera;              // 光源摄像机
    FirstPersonCameraController m_FPSCameraController;              // 摄像机控制器
};


#endif