#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <DirectXColors.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include "d3dApp.h"
#include "BasicFX.h"
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

	BasicFX mBasicFX;										// Basic特效管理类

	CBChangesEveryFrame mCBChangesEveryFrame;				// 该缓冲区存放每帧更新的变量
	CBChangesOnResize mCBChangesOnReSize;					// 该缓冲区存放仅在窗口大小变化时更新的变量
	CBNeverChange mCBNeverChange;							// 该缓冲区存放不会再进行修改的变量
};


#endif