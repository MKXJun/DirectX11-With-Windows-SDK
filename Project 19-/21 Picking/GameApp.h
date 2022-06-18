#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <WinMin.h>
#include "d3dApp.h"
#include "Effects.h"
#include <Camera.h>
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

private:
    bool InitResource();
    
private:

    TextureManager m_TextureManager;
    ModelManager m_ModelManager;

    BasicEffect m_BasicEffect;			                        // 对象渲染特效管理

    std::unique_ptr<Depth2D> m_pDepthTexture;                   // 深度缓冲区

    GameObject m_Sphere;										// 球
    GameObject m_Cube;										    // 立方体
    GameObject m_Cylinder;									    // 圆柱体
    GameObject m_House;										    // 房屋
    GameObject m_Triangle;									    // 三角形
    DirectX::BoundingSphere m_BoundingSphere;				    // 球的包围盒

    GeometryData m_TriangleMesh;						        // 三角形网格模型

    

    std::shared_ptr<FirstPersonCamera> m_pCamera;			    // 摄像机
};


#endif