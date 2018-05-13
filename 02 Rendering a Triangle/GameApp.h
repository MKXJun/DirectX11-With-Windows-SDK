#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <d3dcompiler.h>
#include <directxmath.h>
#include "d3dApp.h"
class GameApp : public D3DApp
{
public:
	struct VertexPosColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
	};

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



private:
	Microsoft::WRL::ComPtr<ID3D11InputLayout> mVertexLayout;	// 顶点输入布局
	Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;			// 顶点缓冲区
	Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;	// 顶点着色器
	Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;		// 像素着色器
};


#endif