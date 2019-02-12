#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Geometry.h"
#include "LightHelper.h"
#include "Camera.h"

class GameApp : public D3DApp
{
public:

	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT4 eyePos;
	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};

	struct CBChangesRarely
	{
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		Material material;
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;		// 打包保证16字节对齐
	};

	// 一个尽可能小的游戏对象类
	class GameObject
	{
	public:
		GameObject();

		// 获取位置
		DirectX::XMFLOAT3 GetPosition() const;
		// 设置缓冲区
		template<class VertexType, class IndexType>
		void SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData);
		// 设置纹理
		void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);
		// 设置矩阵
		void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
		void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX world);
		// 绘制
		void Draw(ComPtr<ID3D11DeviceContext> deviceContext);
	private:
		DirectX::XMFLOAT4X4 mWorldMatrix;				// 世界矩阵
		ComPtr<ID3D11ShaderResourceView> mTexture;		// 纹理
		ComPtr<ID3D11Buffer> mVertexBuffer;				// 顶点缓冲区
		ComPtr<ID3D11Buffer> mIndexBuffer;				// 索引缓冲区
		UINT mVertexStride;								// 顶点字节大小
		UINT mIndexCount;								// 索引数目	
	};

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
	bool InitEffect();
	bool InitResource();

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	ComPtr<ID3D11InputLayout> mVertexLayout2D;				// 用于2D的顶点输入布局
	ComPtr<ID3D11InputLayout> mVertexLayout3D;				// 用于3D的顶点输入布局
	ComPtr<ID3D11Buffer> mConstantBuffers[4];				// 常量缓冲区

	GameObject mWoodCrate;									// 木盒
	GameObject mFloor;										// 地板
	std::vector<GameObject> mWalls;							// 墙壁

	ComPtr<ID3D11VertexShader> mVertexShader3D;				// 用于3D的顶点着色器
	ComPtr<ID3D11PixelShader> mPixelShader3D;				// 用于3D的像素着色器
	ComPtr<ID3D11VertexShader> mVertexShader2D;				// 用于2D的顶点着色器
	ComPtr<ID3D11PixelShader> mPixelShader2D;				// 用于2D的像素着色器

	CBChangesEveryFrame mCBFrame;							// 该缓冲区存放仅在每一帧进行更新的变量
	CBChangesOnResize mCBOnResize;							// 该缓冲区存放仅在窗口大小变化时更新的变量
	CBChangesRarely mCBRarely;								// 该缓冲区存放不会再进行修改的变量

	ComPtr<ID3D11SamplerState> mSamplerState;				// 采样器状态

	std::shared_ptr<Camera> mCamera;						// 摄像机
	CameraMode mCameraMode;									// 摄像机模式

};


#endif