#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include <random>
#include <algorithm>
// 编程捕获帧(需支持DirectX 11.2 API)
//#if defined(DEBUG) | defined(_DEBUG)
//#include <DXGItype.h>  
//#include <dxgi1_2.h>  
//#include <dxgi1_3.h>  
//#include <DXProgrammableCapture.h>  
//#endif

#define BITONIC_BLOCK_SIZE 512

#define TRANSPOSE_BLOCK_SIZE 16

class GameApp : public D3DApp
{
public:
    struct CB
    {
        UINT level;
        UINT descendMask;
        UINT matrixWidth;
        UINT matrixHeight;
    };

public:
    GameApp(HINSTANCE hInstance);
    ~GameApp();

    bool Init();
    void Compute();

private:
    bool InitResource();
    void SetConstants(UINT level, UINT descendMask, UINT matrixWidth, UINT matrixHeight);
    void GPUSort();
private:
    ComPtr<ID3D11Buffer> m_pConstantBuffer;				// 常量缓冲区
    ComPtr<ID3D11Buffer> m_pTypedBuffer1;				// 有类型缓冲区1
    ComPtr<ID3D11Buffer> m_pTypedBuffer2;				// 有类型缓冲区2
    ComPtr<ID3D11Buffer> m_pTypedBufferCopy;			// 用于拷贝的有类型缓冲区
    ComPtr<ID3D11UnorderedAccessView> m_pDataUAV1;		// 有类型缓冲区1对应的无序访问视图
    ComPtr<ID3D11UnorderedAccessView> m_pDataUAV2;		// 有类型缓冲区2对应的无序访问视图
    ComPtr<ID3D11ShaderResourceView> m_pDataSRV1;		// 有类型缓冲区1对应的着色器资源视图
    ComPtr<ID3D11ShaderResourceView> m_pDataSRV2;		// 有类型缓冲区2对应的着色器资源视图

    std::vector<UINT> m_RandomNums;
    UINT m_RandomNumsCount = 0;
    ComPtr<ID3D11ComputeShader> m_pBitonicSort_CS;
    ComPtr<ID3D11ComputeShader> m_pMatrixTranspose_CS;

    GpuTimer m_GpuTimer;
};


#endif