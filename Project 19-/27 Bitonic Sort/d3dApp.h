#ifndef D3DAPP_H
#define D3DAPP_H

#include <wrl/client.h>
#include <string>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "CpuTimer.h"
#include "GpuTimer.h"

class D3DApp
{
public:
    D3DApp(HINSTANCE hInstance);
    virtual ~D3DApp();

    HINSTANCE AppInst()const;       // 获取应用实例的句柄

    int Run();                      // 运行程序，执行消息事件的循环

    // 框架方法。客户派生类需要重载这些方法以实现特定的应用需求
    virtual bool Init();            // 该父类方法需要初始化Direct3D部分
    virtual void Compute() = 0;     // 子类需要实现该方法
    // 窗口的消息回调函数
protected:
    bool InitDirect3D();			// Direct3D初始化

protected:

    HINSTANCE m_hAppInst;			// 应用实例句柄
    CpuTimer m_Timer;               // 计时器

    // 使用模板别名(C++11)简化类型名
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;
    // Direct3D 11
    ComPtr<ID3D11Device> m_pd3dDevice;								// D3D11设备
    ComPtr<ID3D11DeviceContext> m_pd3dImmediateContext;				// D3D11设备上下文
    // Direct3D 11.1
    ComPtr<ID3D11Device1> m_pd3dDevice1;							// D3D11.1设备
    ComPtr<ID3D11DeviceContext1> m_pd3dImmediateContext1;			// D3D11.1设备上下文
};

#endif // D3DAPP_H