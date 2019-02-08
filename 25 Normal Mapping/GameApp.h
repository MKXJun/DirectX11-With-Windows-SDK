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
	// 地面模式
	enum class GroundMode { Floor, Stones };

public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();
	
	void DrawScene(bool drawCenterSphere);

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	ComPtr<ID3D11ShaderResourceView> mFloorDiffuse;			// 地板纹理
	ComPtr<ID3D11ShaderResourceView> mStonesDiffuse;		// 鹅卵石面纹理

	Model mGroundModel;										// 地面网格模型
	Model mGroundTModel;									// 带切线的地面网格模型

	GameObject mSphere;										// 球
	GameObject mGround;										// 地面
	GameObject mGroundT;									// 带切线向量的地面
	GameObject mCylinder;									// 圆柱
	GameObject mCylinderT;									// 带切线向量的圆柱
	GroundMode mGroundMode;									// 地面模式

	ComPtr<ID3D11ShaderResourceView> mBricksNormalMap;		// 砖块法线贴图
	ComPtr<ID3D11ShaderResourceView> mFloorNormalMap;		// 地面法线贴图
	ComPtr<ID3D11ShaderResourceView> mStonesNormalMap;		// 石头地面法线贴图
	bool mEnableNormalMap;									// 开启法线贴图

	BasicEffect mBasicEffect;								// 对象渲染特效管理
	SkyEffect mSkyEffect;									// 天空盒特效管理
	

	std::unique_ptr<DynamicSkyRender> mDaylight;			// 天空盒(白天)

	std::shared_ptr<Camera> mCamera;						// 摄像机
	CameraMode mCameraMode;									// 摄像机模式

	ObjReader mObjReader;									// 模型读取对象
};


#endif