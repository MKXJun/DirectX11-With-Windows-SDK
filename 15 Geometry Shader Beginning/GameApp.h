#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Effects.h"
#include "Vertex.h"

class GameApp : public D3DApp
{
public:
	enum class Mode { SplitedTriangle, CylinderNoCap, CylinderNoCapWithNormal };
	
public:
	GameApp(HINSTANCE hInstance);
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
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	ComPtr<ID3D11Buffer> mVertexBuffer;						// 顶点集合
	int mVertexCount;										// 顶点数目
	Mode mShowMode;											// 当前显示模式

	BasicObjectFX mBasicObjectFX;							// 对象渲染特效管理

};


#endif