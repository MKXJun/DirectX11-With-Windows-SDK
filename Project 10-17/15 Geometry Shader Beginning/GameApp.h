#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <d3dApp.h>
#include <Vertex.h>
#include "Effects.h"


class GameApp : public D3DApp
{
public:
    enum class Mode { SplitedTriangle, CylinderNoCap, CylinderNoCapWithNormal };
    
public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    bool InitResource();

    void ResetTriangle();
    void ResetRoundWire();



private:

    ComPtr<ID3D11Buffer> m_pVertexBuffer;						// 顶点集合
    int m_VertexCount;										    // 顶点数目
    Mode m_ShowMode;											// 当前显示模式

    BasicEffect m_BasicEffect;							        // 对象渲染特效管理

};


#endif