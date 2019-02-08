#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "LightHelper.h"
#include "Geometry.h"

class GameApp : public D3DApp
{
public:

	struct VSConstantBuffer
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX worldInvTranspose;
		
	};

	struct PSConstantBuffer
	{
		DirectionalLight dirLight;
		PointLight pointLight;
		SpotLight spotLight;
		Material material;
		DirectX::XMFLOAT4 eyePos;
	};



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
	bool ResetMesh(const Geometry::MeshData<VertexPosNormalColor>& meshData);


private:
	ComPtr<ID3D11InputLayout> mVertexLayout;	// 顶点输入布局
	ComPtr<ID3D11Buffer> mVertexBuffer;			// 顶点缓冲区
	ComPtr<ID3D11Buffer> mIndexBuffer;			// 索引缓冲区
	ComPtr<ID3D11Buffer> mConstantBuffers[2];	// 常量缓冲区
	UINT mIndexCount;							// 绘制物体的索引数组大小

	ComPtr<ID3D11VertexShader> mVertexShader;	// 顶点着色器
	ComPtr<ID3D11PixelShader> mPixelShader;		// 像素着色器
	VSConstantBuffer mVSConstantBuffer;			// 用于修改用于VS的GPU常量缓冲区的变量
	PSConstantBuffer mPSConstantBuffer;			// 用于修改用于PS的GPU常量缓冲区的变量

	DirectionalLight mDirLight;					// 默认环境光
	PointLight mPointLight;						// 默认点光
	SpotLight mSpotLight;						// 默认汇聚光
	
};


#endif