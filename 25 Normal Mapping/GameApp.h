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
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // 单色笔刷
	ComPtr<IDWriteFont> m_pFont;								// 字体
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// 文本格式

	ComPtr<ID3D11ShaderResourceView> m_FloorDiffuse;			// 地板纹理
	ComPtr<ID3D11ShaderResourceView> m_StonesDiffuse;		    // 鹅卵石面纹理

	Model m_GroundModel;										// 地面网格模型
	Model m_GroundTModel;									    // 带切线的地面网格模型

	GameObject m_Sphere;										// 球
	GameObject m_Ground;										// 地面
	GameObject m_GroundT;									    // 带切线向量的地面
	GameObject m_Cylinder;									    // 圆柱
	GameObject m_CylinderT;									    // 带切线向量的圆柱
	GroundMode m_GroundMode;									// 地面模式

	ComPtr<ID3D11ShaderResourceView> m_BricksNormalMap;		    // 砖块法线贴图
	ComPtr<ID3D11ShaderResourceView> m_FloorNormalMap;		    // 地面法线贴图
	ComPtr<ID3D11ShaderResourceView> m_StonesNormalMap;		    // 石头地面法线贴图
	bool m_EnableNormalMap;									    // 开启法线贴图

	BasicEffect m_BasicEffect;								    // 对象渲染特效管理
	SkyEffect m_SkyEffect;									    // 天空盒特效管理
	

	std::unique_ptr<DynamicSkyRender> m_pDaylight;			    // 天空盒(白天)

	std::shared_ptr<Camera> m_pCamera;						    // 摄像机
	CameraMode m_CameraMode;									// 摄像机模式

	ObjReader m_ObjReader;									    // 模型读取对象
};


#endif