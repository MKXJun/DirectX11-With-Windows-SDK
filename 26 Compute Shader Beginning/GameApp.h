#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
// 编程捕获帧
//#if defined(DEBUG) | defined(_DEBUG)
//#include <DXGItype.h>  
//#include <dxgi1_2.h>  
//#include <dxgi1_3.h>  
//#include <DXProgrammableCapture.h>  
//#endif

class GameApp : public D3DApp
{

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

	ComPtr<ID3D11ComputeShader> mTextureMul_R32G32B32A32_CS;
	ComPtr<ID3D11ComputeShader> mTextureMul_R8G8B8A8_CS;

	ComPtr<ID3D11ShaderResourceView> mTextureInputA;
	ComPtr<ID3D11ShaderResourceView> mTextureInputB;
	
	ComPtr<ID3D11Texture2D> mTextureOutputA;
	ComPtr<ID3D11Texture2D> mTextureOutputB;
	ComPtr<ID3D11UnorderedAccessView> mTextureOutputA_UAV;	
	ComPtr<ID3D11UnorderedAccessView> mTextureOutputB_UAV;
};


#endif