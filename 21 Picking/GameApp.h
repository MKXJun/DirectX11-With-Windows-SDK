#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <DirectXColors.h>
#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
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
	
private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	
	GameObject mSphere;										// 球
	GameObject mCube;										// 立方体
	GameObject mCylinder;									// 圆柱体
	GameObject mHouse;										// 房屋
	GameObject mTriangle;									// 三角形
	DirectX::BoundingSphere mBoundingSphere;				// 球的包围盒

	Geometry::MeshData mTriangleMesh;						// 三角形网格模型

	std::wstring mPickedObjStr;								// 已经拾取的对象名

	BasicEffect mBasicEffect;										// 对象渲染特效管理
	bool mEnableFrustumCulling;								// 视锥体裁剪开启
	bool mEnableInstancing;									// 硬件实例化开启

	std::shared_ptr<Camera> mCamera;						// 摄像机
	CameraMode mCameraMode;									// 摄像机模式

	ObjReader mObjReader;									// 模型读取对象
};


#endif