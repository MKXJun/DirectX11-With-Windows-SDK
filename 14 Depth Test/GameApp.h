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

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	GameObject mBoltAnim;									// 闪电动画
	GameObject mWoodCrate;									// 木盒
	GameObject mFloor;										// 地板
	std::vector<GameObject> mWalls;							// 墙壁
	GameObject mMirror;										// 镜面

	std::vector<ComPtr<ID3D11ShaderResourceView>> mBoltSRVs;// 闪电动画纹理

	Material mShadowMat;									// 阴影材质
	Material mWoodCrateMat;									// 木盒材质

	BasicEffect mBasicEffect;							// 对象渲染特效管理

	std::shared_ptr<Camera> mCamera;						// 摄像机
	CameraMode mCameraMode;									// 摄像机模式

};


#endif