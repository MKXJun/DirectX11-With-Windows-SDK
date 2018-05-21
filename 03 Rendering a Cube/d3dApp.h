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
	D3DApp(HINSTANCE hInstance);    // 在构造函数的初始化列表应当设置好初始参数
	virtual ~D3DApp();

	HINSTANCE AppInst()const;       // 获取应用实例的句柄
	HWND      MainWnd()const;       // 获取主窗口句柄
	float     AspectRatio()const;   // 获取屏幕宽高比

	int Run();                      // 运行程序，执行消息事件的循环

	// 框架方法。客户派生类需要重载这些方法以实现特定的应用需求
	virtual bool Init();            // 该父类方法需要初始化窗口和Direct3D部分
	virtual void OnResize();        // 该父类方法需要在窗口大小变动的时候调用
	virtual void UpdateScene(float dt) = 0;   // 子类需要实现该方法，完成每一帧的更新
	virtual void DrawScene() = 0;             // 子类需要实现该方法，完成每一帧的绘制
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	// 窗口的消息回调函数
protected:
	bool InitMainWindow();      // 窗口初始化
	bool InitDirect3D();        // Direct3D初始化

	void CalculateFrameStats(); // 计算每秒帧数并在窗口显示

protected:

	HINSTANCE mhAppInst;        // 应用实例句柄
	HWND      mhMainWnd;        // 主窗口句柄
	bool      mAppPaused;       // 应用是否暂停
	bool      mMinimized;       // 应用是否最小化
	bool      mMaximized;       // 应用是否最大化
	bool      mResizing;        // 窗口大小是否变化
	UINT      m4xMsaaQuality;   // MSAA支持的质量等级

	GameTimer mTimer;           // 计时器

	// DX11
	Microsoft::WRL::ComPtr<ID3D11Device> md3dDevice;                    // D3D11设备
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> md3dImmediateContext;   // D3D11设备上下文
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;                  // D3D11交换链
	// DX11.1
	Microsoft::WRL::ComPtr<ID3D11Device1> md3dDevice1;                  // D3D11.1设备
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> md3dImmediateContext1; // D3D11.1设备上下文
	Microsoft::WRL::ComPtr<IDXGISwapChain1> mSwapChain1;                // D3D11.1交换链
	// 常用资源
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilBuffer;        // 深度模板缓冲区
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTargetView;   // 渲染目标视图
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView;   // 深度模板视图
	D3D11_VIEWPORT mScreenViewport;                                     // 视口

	// 派生类应该在构造函数设置好这些自定义的初始参数
	std::wstring mMainWndCaption;                                       // 主窗口标题
	int mClientWidth;                                                   // 视口宽度
	int mClientHeight;                                                  // 视口高度
};

#endif // D3DAPP_H