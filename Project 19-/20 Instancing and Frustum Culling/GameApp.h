#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <ctime>
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

private:
    bool InitResource();
    void CreateRandomTrees();
    void CreateRandomCubes();
    
private:
    
    TextureManager m_TextureManager;
    ModelManager m_ModelManager;

    BasicEffect m_BasicEffect;				                            // 对象渲染特效管理

    GpuTimer m_GpuTimer_Instancing;

    std::unique_ptr<Depth2D> m_pDepthTexture;                           // 深度缓冲区

    int m_SceneMode = 0;
    GameObject m_Trees;										            // 树
    GameObject m_Cubes;                                                 // 立方体
    GameObject m_Ground;										        // 地面
    std::vector<Transform> m_TreeTransforms;
    std::vector<Transform> m_CubeTransforms;                            
    std::vector<BasicEffect::InstancedData> m_TreeInstancedData;		// 树的实例数据
    std::vector<BasicEffect::InstancedData> m_CubeInstancedData;		// 立方体的实例数据
    
    std::vector<size_t> m_AcceptedIndices;                              // 通过视锥体裁剪的实例索引
    std::vector<BasicEffect::InstancedData> m_AcceptedData;             // 上传到实例缓冲区的数据
    std::unique_ptr<Buffer> m_pInstancedBuffer;                         // 实例缓冲区

    
    bool m_EnableFrustumCulling = true;							        // 视锥体裁剪开启
    bool m_EnableInstancing = true;								        // 硬件实例化开启

    std::shared_ptr<FirstPersonCamera> m_pCamera;                       // 摄像机
};


#endif