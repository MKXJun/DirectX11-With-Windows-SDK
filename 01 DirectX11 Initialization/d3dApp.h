#ifndef D3DAPP_H
#define D3DAPP_H

#include <wrl/client.h>
#include <assert.h>
#include <d3d11_1.h>
#include <string>
#include "GameTimer.h"
#include "dxerr.h"

// 移植过来的错误检查，该项目仅允许使用Unicode字符集
#if defined(DEBUG) | defined(_DEBUG)
#ifndef HR
#define HR(x)                                              \
{                                                          \
	HRESULT hr = (x);                                      \
	if(FAILED(hr))                                         \
	{                                                      \
		DXTrace(__FILEW__, (DWORD)__LINE__, hr, L#x, true);\
	}                                                      \
}
#endif

#else
#ifndef HR
#define HR(x) (x)
#endif
#endif 

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "winmm.lib")

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();
	
	HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	float     AspectRatio()const;
	
	int Run();
 
	// 框架方法。客户派生类需要重载这些方法以实现特定的应用需求
	virtual bool Init();
	virtual void OnResize(); 
	virtual void UpdateScene(float dt)=0;
	virtual void DrawScene()=0; 
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	bool InitMainWindow();
	bool InitDirect3D();

	void CalculateFrameStats();

protected:

	HINSTANCE mhAppInst;
	HWND      mhMainWnd;
	bool      mAppPaused;
	bool      mMinimized;
	bool      mMaximized;
	bool      mResizing;
	UINT      m4xMsaaQuality;

	GameTimer mTimer;

	// DX11
	Microsoft::WRL::ComPtr<ID3D11Device> md3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> md3dImmediateContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	// DX11.1
	Microsoft::WRL::ComPtr<ID3D11Device1> md3dDevice1;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> md3dImmediateContext1;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> mSwapChain1;
	// 常用资源
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView;
	D3D11_VIEWPORT mScreenViewport;

	// 派生类应该在构造函数设置好这些自定义的初始参数
	std::wstring mMainWndCaption;
	int mClientWidth;
	int mClientHeight;
};

#endif // D3DAPP_H