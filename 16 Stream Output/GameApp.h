#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Effects.h"
#include "Vertex.h"

class GameApp : public D3DApp
{
public:
	enum class Mode { SplitedTriangle, SplitedSnow, SplitedSphere };
	
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

	void ResetSplitedTriangle();
	void ResetSplitedSnow();
	void ResetSplitedSphere();


private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	ComPtr<ID3D11Buffer> mVertexBuffers[7];					// 顶点缓冲区数组
	int mVertexCounts[7];									// 顶点数目
	int mCurrIndex;											// 当前索引
	Mode mShowMode;											// 当前显示模式
	bool mIsWireFrame;										// 是否为线框模式
	bool mShowNormal;										// 是否显示法向量

	BasicEffect mBasicEffect;							// 对象绘制特效管理类

};


#endif