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

// 需要与着色器中的PointLight对应
struct PointLight
{
    DirectX::XMFLOAT3 posV;
    float attenuationBegin;
    DirectX::XMFLOAT3 color;
    float attenuationEnd;
};

// 初始变换数据(柱面坐标系)
struct PointLightInitData
{
    float radius;
    float angle;
    float height;
    float animationSpeed;
};



class GameApp : public D3DApp
{
public:
    // 光源裁剪技术
    enum class LightCullTechnique {
        CULL_FORWARD_NONE = 0,    // 前向渲染，无光照裁剪
        CULL_FORWARD_PREZ_NONE,   // 前向渲染，预写入深度，无光照裁剪
        CULL_DEFERRED_NONE,       // 传统延迟渲染
    };

    const UINT MAX_LIGHTS = 1024;
public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth = 1280, int initHeight = 720);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    bool InitResource();
    void InitLightParams();
    
    DirectX::XMFLOAT3 HueToRGB(float hue);

    void ResizeLights(UINT activeLights);
    void UpdateLights(float dt);
    // 需要在更新帧资源后进行
    void XM_CALLCONV SetupLights(DirectX::XMMATRIX viewMatrix);

    void ResizeBuffers(UINT width, UINT height, UINT msaaSamples);

    void RenderForward(bool doPreZ);
    void RenderGBuffer();
    void RenderSkybox();

private:
    
    // GPU计时
    GpuTimer m_GpuTimer_PreZ;
    GpuTimer m_GpuTimer_Geometry;
    GpuTimer m_GpuTimer_Lighting;
    GpuTimer m_GpuTimer_Skybox;

    // 设置
    LightCullTechnique m_LightCullTechnique = LightCullTechnique::CULL_DEFERRED_NONE;
    bool m_AnimateLights = false;
    bool m_LightingOnly = false;
    bool m_FaceNormals = false;
    bool m_VisualizeLightCount = false;
    bool m_VisualizeShadingFreq = false;
    bool m_ClearGBuffers = false;
    float m_LightHeightScale = 0.25f;


    // 各种资源
    TextureManager m_TextureManager;                                // 纹理读取管理
    ModelManager m_ModelManager;									// 模型读取管理
    UINT m_MsaaSamples = 1;
    bool m_MsaaSamplesChanged = false;
    std::unique_ptr<Texture2DMS> m_pLitBuffer;                      // 场景渲染缓冲区
    std::unique_ptr<Depth2DMS> m_pDepthBuffer;                      // 深度缓冲区
    ComPtr<ID3D11DepthStencilView> m_pDepthBufferReadOnlyDSV;       // 只读深度模板视图
    std::vector<std::unique_ptr<Texture2DMS>> m_pGBuffers;          // G-Buffers
    std::unique_ptr<Texture2D> m_pDebugNormalGBuffer;               // 调试显示用的法线缓冲区
    std::unique_ptr<Texture2D> m_pDebugPosZGradGBuffer;			    // 调试显示用的Z梯度缓冲区
    std::unique_ptr<Texture2D> m_pDebugAlbedoGBuffer;               // 调试显示用的颜色缓冲区

    // 用于一次性传递
    std::vector<ID3D11RenderTargetView*> m_pGBufferRTVs;
    std::vector<ID3D11ShaderResourceView*> m_pGBufferSRVs;

    // 光照
    UINT m_ActiveLights = (MAX_LIGHTS >> 2);
    std::vector<PointLightInitData> m_PointLightInitDatas;
    std::vector<PointLight> m_PointLightParams;
    std::vector<DirectX::XMFLOAT3> m_PointLightPosWorlds;
    std::unique_ptr<StructuredBuffer<PointLight>> m_pLightBuffer;   // 点光源缓冲区

    // 模型
    GameObject m_Sponza;											// 场景模型
    GameObject m_Skybox;											// 天空盒模型

    // 特效
    ForwardEffect m_ForwardEffect;				                    // 前向渲染特效
    DeferredEffect m_DeferredEffect;				                // 延迟渲染特效
    SkyboxEffect m_SkyboxEffect;			                        // 天空盒特效

    // 摄像机
    std::shared_ptr<Camera> m_pCamera;								// 摄像机
    FirstPersonCameraController m_FPSCameraController;				// 摄像机控制器

};


#endif