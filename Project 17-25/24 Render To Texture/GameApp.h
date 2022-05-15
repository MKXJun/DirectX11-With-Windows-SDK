#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "TextureRender.h"
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
    void CreateRandomTrees();

    void DrawScene(bool drawMiniMap);

private:
    
    ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // 单色笔刷
    ComPtr<IDWriteFont> m_pFont;								// 字体
    ComPtr<IDWriteTextFormat> m_pTextFormat;					// 文本格式

    GameObject m_Trees;										    // 树
    GameObject m_Ground;										// 地面
    std::vector<Transform> m_InstancedData;						// 树的实例数据

    BasicEffect m_BasicEffect;								    // 对象渲染特效管理
    ScreenFadeEffect m_ScreenFadeEffect;						// 屏幕淡入淡出特效管理
    MinimapEffect m_MinimapEffect;							    // 小地图特效管理

    std::unique_ptr<TextureRender> m_pMinimapRender;			// 小地图纹理生成
    std::unique_ptr<TextureRender> m_pScreenFadeRender;		    // 截屏纹理生成


    ComPtr<ID3D11Texture2D> m_pMinimapTexture;				    // 小地图纹理
    ComPtr<ID3D11ShaderResourceView> m_pMininmapSRV;			// 小地图着色器资源
    Model m_Minimap;											// 小地图网格模型
    Model m_FullScreenShow;									    // 全屏显示网格模型

    std::unique_ptr<FirstPersonCamera> m_pCamera;				// 摄像机
    std::unique_ptr<FirstPersonCamera> m_MinimapCamera;		    // 小地图所用摄像机
    CameraMode m_CameraMode;									// 摄像机模式

    ObjReader m_ObjReader;									    // 模型读取对象

    bool m_PrintScreenStarted;								    // 截屏当前帧
    bool m_FadeUsed;											// 是否使用淡入/淡出
    float m_FadeAmount;										    // 淡入/淡出系数
    float m_FadeSign;										    // 1.0f表示淡入，-1.0f表示淡出
};


#endif