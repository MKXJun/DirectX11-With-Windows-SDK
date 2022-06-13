#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "ScreenGrab11.h"
#include "DDSTextureLoader11.h"
#include <d3dcompiler.h>

// 编程捕获帧(需支持DirectX 11.2 API)
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

    ComPtr<ID3D11ComputeShader> m_pTextureMul_CS;

    ComPtr<ID3D11ShaderResourceView> m_pTextureInputA;
    ComPtr<ID3D11ShaderResourceView> m_pTextureInputB;
    
    ComPtr<ID3D11Texture2D> m_pTextureOutput;
    ComPtr<ID3D11UnorderedAccessView> m_pTextureOutputUAV;	
};


#endif