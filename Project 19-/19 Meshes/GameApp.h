#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <WinMin.h>
#include "d3dApp.h"
#include "Effects.h"
#include <Camera.h>
#include <RenderStates.h>
#include <GameObject.h>
#include <Texture2D.h>
#include <Buffer.h>
#include <ModelManager.h>
#include <TextureManager.h>

class GameApp : public D3DApp
{
public:
    // 摄像机模式
    enum class CameraMode { FirstPerson, ThirdPerson, Free };
    
public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    bool InitResource();
private:

    TextureManager m_TextureManager;
    ModelManager m_ModelManager;

    std::unique_ptr<Depth2D> m_pDepthBuffer;                    // 深度缓冲区

    GameObject m_House;										    // 房屋
    GameObject m_Ground;										// 地面

    std::unique_ptr<BasicEffect> m_pBasicEffect;                // 对象渲染特效管理

    std::shared_ptr<ThirdPersonCamera> m_pCamera;				// 摄像机
    CameraMode m_CameraMode;									// 摄像机模式
};


#endif