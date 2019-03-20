#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
// ±‡≥Ã≤∂ªÒ÷°
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
	void Compute();

private:
	bool InitResource();

private:

	ComPtr<ID3D11ComputeShader> m_pTextureMul_R32G32B32A32_CS;
	ComPtr<ID3D11ComputeShader> m_pTextureMul_R8G8B8A8_CS;

	ComPtr<ID3D11ShaderResourceView> m_pTextureInputA;
	ComPtr<ID3D11ShaderResourceView> m_pTextureInputB;
	
	ComPtr<ID3D11Texture2D> m_pTextureOutputA;
	ComPtr<ID3D11Texture2D> m_pTextureOutputB;
	ComPtr<ID3D11UnorderedAccessView> m_pTextureOutputA_UAV;	
	ComPtr<ID3D11UnorderedAccessView> m_pTextureOutputB_UAV;
};


#endif