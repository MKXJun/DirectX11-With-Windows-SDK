#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <ctime>
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

#include <ScreenGrab11.h>
#include <wincodec.h> // 使用ScreenGrab11.h需要

class GameApp : public D3DApp
{

public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    bool InitResource();
    void CreateRandomTrees();

    void DrawScene(ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV, const D3D11_VIEWPORT& viewport);

private:

    TextureManager m_TextureManager;
    ModelManager m_ModelManager;

    std::unique_ptr<Depth2D> m_pDepthTexture;                           // 深度缓冲区

    GameObject m_Trees;										            // 树
    GameObject m_Ground;										        // 地面                 
    std::unique_ptr<Buffer> m_pInstancedBuffer;                         // 树的实例缓冲区

    BasicEffect m_BasicEffect;								            // 对象渲染特效管理
    PostProcessEffect m_PostProcessEffect;						        // 后处理特效管理

    std::unique_ptr<Texture2D> m_pMinimapTexture;                       // 小地图纹理
    std::unique_ptr<Texture2D> m_pLitTexture;                           // 中间场景缓冲区

    std::unique_ptr<FirstPersonCamera> m_pCamera;				        // 摄像机

    bool m_PrintScreenStarted = false;						            // 截屏当前帧
    bool m_FadeUsed = true;										        // 是否使用淡入/淡出
    float m_FadeAmount = 0.0f;							                // 淡入/淡出系数
    float m_FadeSign = 1.0f;								            // 1.0f表示淡入，-1.0f表示淡出
};


#endif