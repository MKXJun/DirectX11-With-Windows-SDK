#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "ObjReader.h"
#include "Collision.h"
#include "WavesRender.h"
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

    std::mt19937 m_RandEngine;									// 随机数生成器
    std::uniform_int_distribution<UINT> m_RowRange;				// 行索引范围
    std::uniform_int_distribution<UINT> m_ColRange;				// 列索引范围
    std::uniform_real_distribution<float> m_MagnitudeRange;		// 振幅范围

    BasicEffect m_BasicEffect;									// 对象渲染特效管理

    GameObject m_Land;											// 地面对象
    GameObject m_WireFence;										// 篱笆盒

    std::unique_ptr<CpuWavesRender> m_pCpuWavesRender;			// CPU波浪渲染器
    std::unique_ptr<GpuWavesRender> m_pGpuWavesRender;			// GPU波浪渲染器

    float m_BaseTime;											// 控制水波生成的基准时间

    bool m_EnabledFog;											// 开启雾效
    bool m_EnabledGpuWaves;										// 开启GPU波浪绘制

    std::shared_ptr<Camera> m_pCamera;						    // 摄像机
};


#endif