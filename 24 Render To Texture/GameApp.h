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
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	GameObject mTrees;										// 树
	GameObject mGround;										// 地面
	std::vector<DirectX::XMMATRIX> mInstancedData;			// 树的实例数据
	Collision::WireFrameData mTreeBoxData;					// 树包围盒线框数据

	BasicEffect mBasicEffect;								// 对象渲染特效管理
	ScreenFadeEffect mScreenFadeEffect;						// 屏幕淡入淡出特效管理
	MinimapEffect mMinimapEffect;							// 小地图特效管理

	std::unique_ptr<TextureRender> mMinimapRender;			// 小地图纹理生成
	std::unique_ptr<TextureRender> mScreenFadeRender;		// 截屏纹理生成


	ComPtr<ID3D11Texture2D> mMinimapTexture;				// 小地图纹理
	ComPtr<ID3D11ShaderResourceView> mMininmapSRV;			// 小地图着色器资源
	Model mMinimap;											// 小地图网格模型
	Model mFullScreenShow;									// 全屏显示网格模型

	std::shared_ptr<Camera> mCamera;						// 摄像机
	std::unique_ptr<FirstPersonCamera> mMinimapCamera;		// 小地图所用摄像机
	CameraMode mCameraMode;									// 摄像机模式

	ObjReader mObjReader;									// 模型读取对象

	bool mPrintScreenStarted;								// 截屏当前帧
	bool mEscapePressed;									// 退出键按下
	bool mFadeUsed;											// 是否使用淡入/淡出
	float mFadeAmount;										// 淡入/淡出系数
	float mFadeSign;										// 1.0f表示淡入，-1.0f表示淡出
};


#endif