#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include <random>
// 编程捕获帧
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
	ComPtr<ID3D11Buffer> mConstantBuffer;			// 常量缓冲区
	ComPtr<ID3D11Buffer> mTypedBuffer1;				// 有类型缓冲区1
	ComPtr<ID3D11Buffer> mTypedBuffer2;				// 有类型缓冲区2
	ComPtr<ID3D11Buffer> mTypedBufferCopy;			// 用于拷贝的有类型缓冲区
	ComPtr<ID3D11UnorderedAccessView> mDataUAV1;	// 有类型缓冲区1对应的无序访问视图
	ComPtr<ID3D11UnorderedAccessView> mDataUAV2;	// 有类型缓冲区2对应的无序访问视图
	ComPtr<ID3D11ShaderResourceView> mDataSRV1;		// 有类型缓冲区1对应的着色器资源视图
	ComPtr<ID3D11ShaderResourceView> mDataSRV2;		// 有类型缓冲区2对应的着色器资源视图

	std::vector<UINT> mRandomNums;
	UINT mRandomNumsCount;
	ComPtr<ID3D11ComputeShader> mBitonicSort_CS;
	ComPtr<ID3D11ComputeShader> mMatrixTranspose_CS;
};


#endif