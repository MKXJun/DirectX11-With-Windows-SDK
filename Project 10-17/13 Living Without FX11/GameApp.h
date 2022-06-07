#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <d3dApp.h>
#include <Camera.h>
#include "GameObject.h"


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

    GameObject m_WoodCrate;									    // 木盒
    GameObject m_Floor;										    // 地板
    std::vector<GameObject> m_Walls;							// 墙壁
    GameObject m_Mirror;										// 镜面

    Material m_ShadowMat;									    // 阴影材质
    Material m_WoodCrateMat;									// 木盒材质

    BasicEffect m_BasicEffect;								    // 对象渲染特效管理

    std::shared_ptr<Camera> m_pCamera;						    // 摄像机
    CameraMode m_CameraMode;									// 摄像机模式

};


#endif