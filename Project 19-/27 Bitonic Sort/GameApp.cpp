#include "GameApp.h"

using namespace DirectX;

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
    assert(m_pd3dImmediateContext);

//#if defined(DEBUG) | defined(_DEBUG)
//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
//	graphicsAnalysis->BeginCapture();
//#endif

    // GPU排序
    m_Timer.Reset();
    m_Timer.Start();
    m_GpuTimer.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    m_GpuTimer.Start();
    GPUSort();
    m_GpuTimer.Stop();
    double gpuComputeTime = m_GpuTimer.GetTime();

    // 结果回读到CPU进行比较
    m_pd3dImmediateContext->CopyResource(m_pTypedBufferCopy.Get(), m_pTypedBuffer1.Get());
    D3D11_MAPPED_SUBRESOURCE mappedData;
    m_pd3dImmediateContext->Map(m_pTypedBufferCopy.Get(), 0, D3D11_MAP_READ, 0, &mappedData);
    m_Timer.Tick();
    m_Timer.Stop();
    float gpuTotalTime = m_Timer.TotalTime();

//#if defined(DEBUG) | defined(_DEBUG)
//	graphicsAnalysis->EndCapture();
//#endif

    // CPU排序
    m_Timer.Reset();
    m_Timer.Start();
    std::sort(m_RandomNums.begin(), m_RandomNums.begin() + m_RandomNumsCount);
    m_Timer.Tick();
    m_Timer.Stop();
    float cpuTotalTime = m_Timer.TotalTime();

    bool isSame = !memcmp(mappedData.pData, m_RandomNums.data(),
        sizeof(uint32_t) * m_RandomNums.size());

    m_pd3dImmediateContext->Unmap(m_pTypedBufferCopy.Get(), 0);

    std::wstring wstr = L"排序元素数目：" + std::to_wstring(m_RandomNumsCount) +
        L"/" + std::to_wstring(m_RandomNums.size());
    wstr += L"\nGPU计算用时：" + std::to_wstring(gpuComputeTime) + L"秒";
    wstr += L"\nGPU总用时：" + std::to_wstring(gpuTotalTime) + L"秒";
    wstr += L"\nCPU用时：" + std::to_wstring(cpuTotalTime) + L"秒";
    wstr += isSame ? L"\n排序结果一致" : L"\n排序结果不一致";
    MessageBox(nullptr, wstr.c_str(), L"排序结束", MB_OK);

}



bool GameApp::InitResource()
{
    // 初始化随机数数据
    std::mt19937 randEngine;
    randEngine.seed(std::random_device()());
    std::uniform_int_distribution<uint32_t> powRange(9, 18);
    // 元素数目必须为2的次幂且不小于512个，并用最大值填充
    uint32_t elemCount = 1 << 18;
    m_RandomNums.assign(elemCount, UINT_MAX);
    // 填充随机数目的随机数，数目在一半容量到最大容量之间
    std::uniform_int_distribution<uint32_t> numsCountRange((uint32_t)m_RandomNums.size() / 2,
        (uint32_t)m_RandomNums.size());
    m_RandomNumsCount = elemCount;
    std::generate(m_RandomNums.begin(), m_RandomNums.begin() + m_RandomNumsCount, [&] {return randEngine(); });

    CD3D11_BUFFER_DESC bufferDesc(
        (uint32_t)m_RandomNums.size() * sizeof(uint32_t),
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = m_RandomNums.data();
    m_pd3dDevice->CreateBuffer(&bufferDesc, &initData, m_pTypedBuffer1.GetAddressOf());
    m_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, m_pTypedBuffer2.GetAddressOf());
    
    bufferDesc.BindFlags = 0;
    bufferDesc.Usage = D3D11_USAGE_STAGING;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    m_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, m_pTypedBufferCopy.GetAddressOf());

    bufferDesc = CD3D11_BUFFER_DESC(sizeof(CB), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    m_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, m_pConstantBuffer.GetAddressOf());

    // 创建着色器资源视图
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_BUFFER, DXGI_FORMAT_R32_UINT, 0, (uint32_t)m_RandomNums.size());
    m_pd3dDevice->CreateShaderResourceView(m_pTypedBuffer1.Get(), &srvDesc,
        m_pDataSRV1.GetAddressOf());
    m_pd3dDevice->CreateShaderResourceView(m_pTypedBuffer2.Get(), &srvDesc,
        m_pDataSRV2.GetAddressOf());

    // 创建无序访问视图
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = (UINT)m_RandomNums.size();
    m_pd3dDevice->CreateUnorderedAccessView(m_pTypedBuffer1.Get(), &uavDesc,
        m_pDataUAV1.GetAddressOf());
    m_pd3dDevice->CreateUnorderedAccessView(m_pTypedBuffer2.Get(), &uavDesc,
        m_pDataUAV2.GetAddressOf());

    // 创建计算着色器
    ComPtr<ID3DBlob> blob;
    D3DReadFileToBlob(L"Shaders\\BitonicSort_CS.cso", blob.ReleaseAndGetAddressOf());
    m_pd3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pBitonicSort_CS.GetAddressOf());

    D3DReadFileToBlob(L"Shaders\\MatrixTranspose_CS.cso", blob.ReleaseAndGetAddressOf());
    m_pd3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pMatrixTranspose_CS.GetAddressOf());


    return true;
}

void GameApp::SetConstants(UINT level, UINT descendMask, UINT matrixWidth, UINT matrixHeight)
{
    D3D11_MAPPED_SUBRESOURCE mappedData{};
    m_pd3dImmediateContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
    CB cb = { level, descendMask, matrixWidth, matrixHeight };
    memcpy_s(mappedData.pData, sizeof cb, &cb, sizeof cb);
    m_pd3dImmediateContext->Unmap(m_pConstantBuffer.Get(), 0);
    m_pd3dImmediateContext->CSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
}

void GameApp::GPUSort()
{
    UINT size = (UINT)m_RandomNums.size();

    m_pd3dImmediateContext->CSSetShader(m_pBitonicSort_CS.Get(), nullptr, 0);
    m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pDataUAV1.GetAddressOf(), nullptr);

    // 按行数据进行排序，先排序level <= BLOCK_SIZE 的所有情况
    for (UINT level = 2; level <= size && level <= BITONIC_BLOCK_SIZE; level *= 2)
    {
        SetConstants(level, level, 0, 0);
        m_pd3dImmediateContext->Dispatch((size + BITONIC_BLOCK_SIZE - 1) / BITONIC_BLOCK_SIZE, 1, 1);
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
        m_pd3dImmediateContext->CSSetShader(m_pMatrixTranspose_CS.Get(), nullptr, 0);
        m_pd3dImmediateContext->CSSetShaderResources(0, 1, pNullSRV.GetAddressOf());
        m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pDataUAV2.GetAddressOf(), nullptr);
        m_pd3dImmediateContext->CSSetShaderResources(0, 1, m_pDataSRV1.GetAddressOf());
        m_pd3dImmediateContext->Dispatch(matrixWidth / TRANSPOSE_BLOCK_SIZE, 
            matrixHeight / TRANSPOSE_BLOCK_SIZE, 1);

        // 对Buffer2排序列数据
        m_pd3dImmediateContext->CSSetShader(m_pBitonicSort_CS.Get(), nullptr, 0);
        m_pd3dImmediateContext->Dispatch(size / BITONIC_BLOCK_SIZE, 1, 1);

        // 接着转置回来，并把数据输出到Buffer1
        SetConstants(matrixWidth, level, matrixWidth, matrixHeight);
        m_pd3dImmediateContext->CSSetShader(m_pMatrixTranspose_CS.Get(), nullptr, 0);
        m_pd3dImmediateContext->CSSetShaderResources(0, 1, pNullSRV.GetAddressOf());
        m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pDataUAV1.GetAddressOf(), nullptr);
        m_pd3dImmediateContext->CSSetShaderResources(0, 1, m_pDataSRV2.GetAddressOf());
        m_pd3dImmediateContext->Dispatch(matrixWidth / TRANSPOSE_BLOCK_SIZE,
            matrixHeight / TRANSPOSE_BLOCK_SIZE, 1);

        // 对Buffer1排序剩余行数据
        m_pd3dImmediateContext->CSSetShader(m_pBitonicSort_CS.Get(), nullptr, 0);
        m_pd3dImmediateContext->Dispatch(size / BITONIC_BLOCK_SIZE, 1, 1);
    }
}



