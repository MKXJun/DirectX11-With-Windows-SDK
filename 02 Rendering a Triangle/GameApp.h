#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <d3dcompiler.h>
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

	// objFileNameInOut为编译好的着色器二进制文件(.*so)，若有指定则优先寻找该文件并读取
	// hlslFileName为着色器代码，若未找到着色器二进制文件则编译着色器代码
	// 编译成功后，若指定了objFileNameInOut，则保存编译好的着色器二进制信息到该文件
	// ppBlobOut输出着色器二进制信息
	HRESULT CreateShaderFromFile(const WCHAR* objFileNameInOut, const WCHAR* hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppBlobOut);

private:
	bool InitEffect();
	bool InitResource();



private:
	ComPtr<ID3D11InputLayout> mVertexLayout;	// 顶点输入布局
	ComPtr<ID3D11Buffer> mVertexBuffer;			// 顶点缓冲区
	ComPtr<ID3D11VertexShader> mVertexShader;	// 顶点着色器
	ComPtr<ID3D11PixelShader> mPixelShader;		// 像素着色器
};


#endif