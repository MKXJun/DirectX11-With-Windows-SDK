#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <DirectXColors.h>
#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "ObjReader.h"
#include "Collision.h"
class GameApp : public D3DApp
{
public:
    // 摄像机模式
    enum class CameraMode { FirstPerson, ThirdPerson, Free };
    
public:
    GameApp(HINSTANCE hInstance);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    bool InitResource();
    
private:
    
    ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // 单色笔刷
    ComPtr<IDWriteFont> m_pFont;								// 字体
    ComPtr<IDWriteTextFormat> m_pTextFormat;					// 文本格式

    
    GameObject m_Sphere;										// 球
    GameObject m_Cube;										    // 立方体
    GameObject m_Cylinder;									    // 圆柱体
    GameObject m_House;										    // 房屋
    GameObject m_Triangle;									    // 三角形
    DirectX::BoundingSphere m_BoundingSphere;				    // 球的包围盒

    Geometry::MeshData<> m_TriangleMesh;						// 三角形网格模型

    std::wstring m_PickedObjStr;								// 已经拾取的对象名

    BasicEffect m_BasicEffect;								    // 对象渲染特效管理

    std::shared_ptr<Camera> m_pCamera;						    // 摄像机
    CameraMode m_CameraMode;									// 摄像机模式

    ObjReader m_ObjReader;									    // 模型读取对象
};


#endif