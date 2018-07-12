#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <d3dcompiler.h>
#include <directxmath.h>
#include <DirectXColors.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include "d3dApp.h"
#include "LightHelper.h"
#include "Geometry.h"

class GameApp : public D3DApp
{
public:
	struct VertexPosNormalTex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 tex;
		static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
	};

	

	struct VSConstantBuffer
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX worldInvTranspose;
		
	};

	struct PSConstantBuffer
	{
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		Material material;
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;		// 打包保证16字节对齐
		DirectX::XMFLOAT4 eyePos;
	};

	enum class ShowMode { WoodCrate, FireAnim };

public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	// 从.fx/.hlsl文件中编译着色器
	HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

private:
	bool InitEffect();
	bool InitResource();
	bool ResetMesh(const Geometry::MeshData& meshData);


private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// 单色笔刷
	ComPtr<IDWriteFont> mFont;								// 字体
	ComPtr<IDWriteTextFormat> mTextFormat;					// 文本格式

	ComPtr<ID3D11InputLayout> mVertexLayout2D;				// 用于2D的顶点输入布局
	ComPtr<ID3D11InputLayout> mVertexLayout3D;				// 用于3D的顶点输入布局
	ComPtr<ID3D11Buffer> mVertexBuffer;						// 顶点缓冲区
	ComPtr<ID3D11Buffer> mIndexBuffer;						// 索引缓冲区
	ComPtr<ID3D11Buffer> mConstantBuffers[2];				// 常量缓冲区
	int mIndexCount;										// 绘制物体的索引数组大小
	int mCurrFrame;											// 当前火焰动画播放到第几帧
	ShowMode mCurrMode;										// 当前显示的模式

	ComPtr<ID3D11ShaderResourceView> mWoodCrate;			// 木盒纹理
	std::vector<ComPtr<ID3D11ShaderResourceView>> mFireAnim;// 火焰纹理集
	ComPtr<ID3D11SamplerState> mSamplerState;				// 采样器状态

	ComPtr<ID3D11VertexShader> mVertexShader3D;				// 用于3D的顶点着色器
	ComPtr<ID3D11PixelShader> mPixelShader3D;				// 用于3D的像素着色器
	ComPtr<ID3D11VertexShader> mVertexShader2D;				// 用于2D的顶点着色器
	ComPtr<ID3D11PixelShader> mPixelShader2D;				// 用于2D的像素着色器

	VSConstantBuffer mVSConstantBuffer;						// 用于修改用于VS的GPU常量缓冲区的变量
	PSConstantBuffer mPSConstantBuffer;						// 用于修改用于PS的GPU常量缓冲区的变量
};


#endif