#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <WinMin.h>
#include <d3dApp.h>
#include <EffectHelper.h>
#include <sstream>

class GameApp : public D3DApp
{
public:
    enum class TessellationMode { Triangle, Quad, BezierCurve, BezierSurface };
    enum class PartitionMode { Integer, Odd, Even };
public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

    void UpdateTriangle();
    void UpdateQuad();
    void UpdateBezierCurve();
    void UpdateBezierSurface();

    void DrawTriangle();
    void DrawQuad();
    void DrawBezierCurve();
    void DrawBezierSurface();

private:
    bool InitResource();

private:
    
    TessellationMode m_TessellationMode = TessellationMode::Triangle;	// 镶嵌模式
    PartitionMode m_PartitionMode = PartitionMode::Integer;				// 细分模式

    int m_ChosenBezPoint = -1;											// 被选中的控制点
    ComPtr<ID3D11Buffer> m_pBezCurveVB;									// 贝塞尔曲线缓冲区
    DirectX::XMFLOAT3 m_BezPoints[10] = {};								// 贝塞尔控制点
    float m_IsolineEdgeTess[2] = { 1.0f, 64.0f };						// 未知参数和段数

    ComPtr<ID3D11Buffer> m_pTriangleVB;									// 三角形顶点缓冲区
    float m_TriEdgeTess[3] = { 4.0f, 4.0f, 4.0f };						// 三角形各边的细分因子
    float m_TriInsideTess = 4.0f;										// 三角形内部的细分因子

    ComPtr<ID3D11Buffer> m_pQuadVB;										// 四边形顶点缓冲区
    float m_QuadEdgeTess[4] = { 4.0f, 4.0f, 4.0f, 4.0f };				// 四边形各边的细分因子
    float m_QuadInsideTess[2] = { 4.0f, 4.0f };							// 四边形内部的细分因子

    ComPtr<ID3D11Buffer> m_pBezSurfaceVB;								// 贝塞尔曲面缓冲区
    float m_QuadPatchEdgeTess[4] = { 25.0f, 25.0f, 25.0f, 25.0f };		// 贝塞尔曲面各边的细分因子
    float m_QuadPatchInsideTess[2] = { 25.0f, 25.0f };					// 贝塞尔曲面内部的细分因子
    float m_Phi = 3.14159f / 4, m_Theta = 0.0f, m_Radius = 30.0f;		// 第三人称视角摄像机参数

    ComPtr<ID3D11InputLayout> m_pInputLayout;							// 顶点输入布局
    ComPtr<ID3D11RasterizerState> m_pRSWireFrame;						// 线框光栅化

    std::unique_ptr<EffectHelper> m_pEffectHelper;						// 特效助理
};


#endif