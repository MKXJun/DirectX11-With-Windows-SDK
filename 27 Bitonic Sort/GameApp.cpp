#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"

using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	if (!InitResource())
		return false;

	return true;
}

void GameApp::Compute()
{
	assert(md3dImmediateContext);

//#if defined(DEBUG) | defined(_DEBUG)
//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
//	graphicsAnalysis->BeginCapture();
//#endif

	// GPU排序
	mTimer.Reset();
	mTimer.Start();
	GPUSort();
	mTimer.Tick();
	mTimer.Stop();
	float gpuTotalTime = mTimer.TotalTime();

	// 结果拷贝出来
	md3dImmediateContext->CopyResource(mTypedBufferCopy.Get(), mTypedBuffer1.Get());
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(md3dImmediateContext->Map(mTypedBufferCopy.Get(), 0, D3D11_MAP_READ, 0, &mappedData));

//#if defined(DEBUG) | defined(_DEBUG)
//	graphicsAnalysis->EndCapture();
//#endif

	// CPU排序
	mTimer.Reset();
	mTimer.Start();
	std::sort(mRandomNums.begin(), mRandomNums.begin() + mRandomNumsCount);
	mTimer.Tick();
	mTimer.Stop();
	float cpuTotalTime = mTimer.TotalTime();

	bool isSame = !memcmp(mappedData.pData, mRandomNums.data(),
		sizeof(UINT) * mRandomNums.size());

	md3dImmediateContext->Unmap(mTypedBufferCopy.Get(), 0);

	std::wstring wstr = L"排序元素数目：" + std::to_wstring(mRandomNumsCount) +
		L"/" + std::to_wstring(mRandomNums.size());
	wstr += L"\nGPU用时：" + std::to_wstring(gpuTotalTime) + L"秒";
	wstr += L"\nCPU用时：" + std::to_wstring(cpuTotalTime) + L"秒";
	wstr += isSame ? L"\n排序结果一致" : L"\n排序结果不一致";
	MessageBox(nullptr, wstr.c_str(), L"排序结束", MB_OK);

}



bool GameApp::InitResource()
{
	// 初始化随机数数据
	std::mt19937 randEngine;
	randEngine.seed(std::random_device()());
	std::uniform_int_distribution<UINT> powRange(9, 18);
	// 元素数目必须为2的次幂且不小于512个，并用最大值填充
	mRandomNums.assign((UINT)(1 << powRange(randEngine)), UINT_MAX);
	// 填充随机数目的随机数，数目在一半容量到最大容量之间
	std::uniform_int_distribution<UINT> numsCountRange((UINT)mRandomNums.size() / 2,
		(UINT)mRandomNums.size());
	mRandomNumsCount = numsCountRange(randEngine);
	std::generate(mRandomNums.begin(), mRandomNums.begin() + mRandomNumsCount, [&] {return randEngine(); });

	HR(CreateTypedBuffer(md3dDevice.Get(), mRandomNums.data(), (UINT)mRandomNums.size() * sizeof(UINT),
		mTypedBuffer1.GetAddressOf(), false, true));
	
	HR(CreateTypedBuffer(md3dDevice.Get(), nullptr, (UINT)mRandomNums.size() * sizeof(UINT),
		mTypedBuffer2.GetAddressOf(), false, true));

	HR(CreateTypedBuffer(md3dDevice.Get(), nullptr, (UINT)mRandomNums.size() * sizeof(UINT),
		mTypedBufferCopy.GetAddressOf(), true, true));

	HR(CreateConstantBuffer(md3dDevice.Get(), nullptr, sizeof(CB),
		mConstantBuffer.GetAddressOf(), false, true));

	// 创建着色器资源视图
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_UINT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = (UINT)mRandomNums.size();
	HR(md3dDevice->CreateShaderResourceView(mTypedBuffer1.Get(), &srvDesc,
		mDataSRV1.GetAddressOf()));
	HR(md3dDevice->CreateShaderResourceView(mTypedBuffer2.Get(), &srvDesc,
		mDataSRV2.GetAddressOf()));

	// 创建无序访问视图
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_R32_UINT;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = (UINT)mRandomNums.size();
	HR(md3dDevice->CreateUnorderedAccessView(mTypedBuffer1.Get(), &uavDesc,
		mDataUAV1.GetAddressOf()));
	HR(md3dDevice->CreateUnorderedAccessView(mTypedBuffer2.Get(), &uavDesc,
		mDataUAV2.GetAddressOf()));

	// 创建计算着色器
	ComPtr<ID3DBlob> blob;
	HR(CreateShaderFromFile(L"HLSL\\BitonicSort_CS.cso",
		L"HLSL\\BitonicSort_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf()));
	HR(md3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mBitonicSort_CS.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\MatrixTranspose_CS.cso",
		L"HLSL\\MatrixTranspose_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf()));
	HR(md3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mMatrixTranspose_CS.GetAddressOf()));

	return true;
}

void GameApp::SetConstants(UINT level, UINT descendMask, UINT matrixWidth, UINT matrixHeight)
{
	CB cb = { level, descendMask, matrixWidth, matrixHeight };
	md3dImmediateContext->UpdateSubresource(mConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	md3dImmediateContext->CSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
}

void GameApp::GPUSort()
{
	UINT size = (UINT)mRandomNums.size();

	md3dImmediateContext->CSSetShader(mBitonicSort_CS.Get(), nullptr, 0);
	md3dImmediateContext->CSSetUnorderedAccessViews(0, 1, mDataUAV1.GetAddressOf(), nullptr);

	// 按行数据进行排序，先排序level <= BLOCK_SIZE 的所有情况
	for (UINT level = 2; level <= size && level <= BITONIC_BLOCK_SIZE; level *= 2)
	{
		SetConstants(level, level, 0, 0);
		md3dImmediateContext->Dispatch((size + BITONIC_BLOCK_SIZE - 1) / BITONIC_BLOCK_SIZE, 1, 1);
	}
	
	// 计算相近的矩阵宽高(宽>=高且需要都为2的次幂)
	UINT matrixWidth = 2, matrixHeight = 2;
	while (matrixWidth * matrixWidth < size)
	{
		matrixWidth *= 2;
	}
	matrixHeight = size / matrixWidth;

	// 排序level > BLOCK_SIZE 的所有情况
	ComPtr<ID3D11ShaderResourceView> pNullSRV;
	for (UINT level = BITONIC_BLOCK_SIZE * 2; level <= size; level *= 2)
	{
		// 如果达到最高等级，则为全递增序列
		if (level == size)
		{
			SetConstants(level / matrixWidth, level, matrixWidth, matrixHeight);
		}
		else
		{
			SetConstants(level / matrixWidth, level / matrixWidth, matrixWidth, matrixHeight);
		}
		// 先进行转置，并把数据输出到Buffer2
		md3dImmediateContext->CSSetShader(mMatrixTranspose_CS.Get(), nullptr, 0);
		md3dImmediateContext->CSSetShaderResources(0, 1, pNullSRV.GetAddressOf());
		md3dImmediateContext->CSSetUnorderedAccessViews(0, 1, mDataUAV2.GetAddressOf(), nullptr);
		md3dImmediateContext->CSSetShaderResources(0, 1, mDataSRV1.GetAddressOf());
		md3dImmediateContext->Dispatch(matrixWidth / TRANSPOSE_BLOCK_SIZE, 
			matrixHeight / TRANSPOSE_BLOCK_SIZE, 1);

		// 对Buffer2排序列数据
		md3dImmediateContext->CSSetShader(mBitonicSort_CS.Get(), nullptr, 0);
		md3dImmediateContext->Dispatch(size / BITONIC_BLOCK_SIZE, 1, 1);

		// 接着转置回来，并把数据输出到Buffer1
		SetConstants(matrixWidth, level, matrixWidth, matrixHeight);
		md3dImmediateContext->CSSetShader(mMatrixTranspose_CS.Get(), nullptr, 0);
		md3dImmediateContext->CSSetShaderResources(0, 1, pNullSRV.GetAddressOf());
		md3dImmediateContext->CSSetUnorderedAccessViews(0, 1, mDataUAV1.GetAddressOf(), nullptr);
		md3dImmediateContext->CSSetShaderResources(0, 1, mDataSRV2.GetAddressOf());
		md3dImmediateContext->Dispatch(matrixWidth / TRANSPOSE_BLOCK_SIZE,
			matrixHeight / TRANSPOSE_BLOCK_SIZE, 1);

		// 对Buffer1排序剩余行数据
		md3dImmediateContext->CSSetShader(mBitonicSort_CS.Get(), nullptr, 0);
		md3dImmediateContext->Dispatch(size / BITONIC_BLOCK_SIZE, 1, 1);
	}
}



