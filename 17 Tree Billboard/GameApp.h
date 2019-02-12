#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"


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
	void InitPointSpritesBuffer();

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	ComPtr<ID3D11Buffer> mPointSpritesBuffer;				// 点精灵顶点缓冲区
	ComPtr<ID3D11ShaderResourceView> mTreeTexArray;			// 树的纹理数组
	Material mTreeMat;										// 树的材质

	GameObject mGround;										// 地面
	
	BasicEffect mBasicEffect;							// 对象渲染特效管理

	CameraMode mCameraMode;									// 摄像机模式
	std::shared_ptr<Camera> mCamera;						// 摄像机

	bool mFogEnabled;										// 是否开启雾效
	bool mIsNight;											// 是否黑夜
	bool mEnableAlphaToCoverage;							// 是否开启Alpha-To-Coverage

	float mFogRange;										// 雾效范围
};


#endif