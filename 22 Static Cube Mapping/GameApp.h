#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "SkyRender.h"
#include "ObjReader.h"
#include "Collision.h"
class GameApp : public D3DApp
{
public:
	// 摄像机模式
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
	// 天空盒模式
	enum class SkyBoxMode { Daylight, Sunset, Desert };

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

	GameObject mSphere;										// 球
	GameObject mGround;										// 地面
	GameObject mCylinder;									// 圆柱

	BasicEffect mBasicEffect;								// 对象渲染特效管理
	
	SkyEffect mSkyEffect;									// 天空盒特效管理
	std::unique_ptr<SkyRender> mDaylight;					// 天空盒(白天)
	std::unique_ptr<SkyRender> mSunset;						// 天空盒(日落)
	std::unique_ptr<SkyRender> mDesert;						// 天空盒(沙漠)
	SkyBoxMode mSkyBoxMode;									// 天空盒模式

	std::shared_ptr<Camera> mCamera;						// 摄像机
	CameraMode mCameraMode;									// 摄像机模式

	ObjReader mObjReader;									// 模型读取对象
};


#endif