#include "WavesRender.h"
#include "Geometry.h"
#include "d3dUtil.h"

using namespace DirectX;
using namespace Microsoft::WRL;

void WavesRender::SetMaterial(const Material& material)
{
	m_Material = material;
}

void XM_CALLCONV WavesRender::SetWorldMatrix(DirectX::FXMMATRIX world)
{
	XMStoreFloat4x4(&m_WorldMatrix, world);
}

UINT WavesRender::RowCount() const
{
	return m_NumRows;
}

UINT WavesRender::ColumnCount() const
{
	return m_NumCols;
}

void WavesRender::Init(UINT rows, UINT cols, float texU, float texV,
	float timeStep, float spatialStep, float waveSpeed, float damping, float flowSpeedX, float flowSpeedY)
{
	XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());

	m_NumRows = rows;
	m_NumCols = cols;

	m_TexU = texU;
	m_TexV = texV;
	m_TexOffset = XMFLOAT2();

	m_VertexCount = rows * cols;
	m_IndexCount = 6 * (rows - 1) * (cols - 1);

	m_TimeStep = timeStep;
	m_SpatialStep = spatialStep;
	m_FlowSpeedX = flowSpeedX;
	m_FlowSpeedY = flowSpeedY;
	m_AccumulateTime = 0.0f;

	float d = damping * timeStep + 2.0f;
	float e = (waveSpeed * waveSpeed) * (timeStep * timeStep) / (spatialStep * spatialStep);
	m_K1 = (damping * timeStep - 2.0f) / d;
	m_K2 = (4.0f - 8.0f * e) / d;
	m_K3 = (2.0f * e) / d;
}

HRESULT CpuWavesRender::InitResource(ID3D11Device* device, const std::wstring& texFileName,
	UINT rows, UINT cols, float texU, float texV, float timeStep, float spatialStep, float waveSpeed, float damping,
	float flowSpeedX, float flowSpeedY)
{
	// 防止重复初始化造成内存泄漏
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();
	m_pTextureDiffuse.Reset();

	// 初始化水波数据
	Init(rows, cols, texU, texV, timeStep, spatialStep, waveSpeed, damping, flowSpeedX, flowSpeedY);

	m_isUpdated = false;

	// 顶点行(列)数 - 1 = 网格行(列)数
	auto meshData = Geometry::CreateTerrain<VertexPosNormalTex, DWORD>(XMFLOAT2((cols - 1) * spatialStep, (rows - 1) * spatialStep),
		XMUINT2(cols - 1, rows - 1));

	HRESULT hr;

	// 创建动态顶点缓冲区
	hr = CreateVertexBuffer(device, meshData.vertexVec.data(), (UINT)meshData.vertexVec.size() * sizeof(VertexPosNormalTex),
		m_pVertexBuffer.GetAddressOf(), true);
	if (FAILED(hr))
		return hr;
	// 创建索引缓冲区
	hr = CreateIndexBuffer(device, meshData.indexVec.data(), (UINT)meshData.indexVec.size() * sizeof(DWORD),
		m_pIndexBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 取出顶点数据
	m_Vertices.swap(meshData.vertexVec);
	// 保持与顶点数目一致
	m_PrevSolution.resize(m_Vertices.size());
	// 顶点位置复制到Prev
	for (size_t i = 0; i < m_NumRows; ++i)
	{
		for (size_t j = 0; j < m_NumCols; ++j)
		{
			m_PrevSolution[i * (size_t)m_NumCols + j].pos = m_Vertices[i * (size_t)m_NumCols + j].pos;
		}
	}

	// 读取纹理
	if (texFileName.size() > 4)
	{
		if (texFileName.substr(texFileName.size() - 3, 3) == L"dds")
		{
			hr = CreateDDSTextureFromFile(device, texFileName.c_str(), nullptr,
				m_pTextureDiffuse.GetAddressOf());
		}
		else
		{
			hr = CreateWICTextureFromFile(device, texFileName.c_str(), nullptr,
				m_pTextureDiffuse.GetAddressOf());
		}
	}
	return hr;
}

void CpuWavesRender::Update(float dt)
{
	m_AccumulateTime += dt;
	m_TexOffset.x += m_FlowSpeedX * dt;
	m_TexOffset.y += m_FlowSpeedY * dt;

	// 仅仅在累积时间大于时间步长时才更新
	if (m_AccumulateTime > m_TimeStep)
	{
		m_isUpdated = true;
		// 仅仅对内部顶点进行更新
		for (size_t i = 1; i < m_NumRows - 1; ++i)
		{
			for (size_t j = 1; j < m_NumCols - 1; ++j)
			{
				// 在这次更新之后，我们将丢弃掉上一次模拟的数据。
				// 因此我们将运算的结果保存到Prev[i][j]的位置上。
				// 注意我们能够使用这种原址更新是因为Prev[i][j]
				// 的数据仅在当前计算Next[i][j]的时候才用到
				m_PrevSolution[i * m_NumCols + j].pos.y =
					m_K1 * m_PrevSolution[i * m_NumCols + j].pos.y +
					m_K2 * m_Vertices[i * m_NumCols + j].pos.y +
					m_K3 * (m_Vertices[(i + 1) * m_NumCols + j].pos.y +
						m_Vertices[(i - 1) * m_NumCols + j].pos.y +
						m_Vertices[i * m_NumCols + j + 1].pos.y +
						m_Vertices[i * m_NumCols + j - 1].pos.y);
			}
		}

		// 由于把下一次模拟的结果写到了上一次模拟的缓冲区内，
		// 我们需要将下一次模拟的结果与当前模拟的结果交换
		for (size_t i = 1; i < m_NumRows - 1; ++i)
		{
			for (size_t j = 1; j < m_NumCols - 1; ++j)
			{
				std::swap(m_PrevSolution[i * m_NumCols + j].pos, m_Vertices[i * m_NumCols + j].pos);
			}
		}

		m_AccumulateTime = 0.0f;	// 重置时间

		// 使用有限差分法计算法向量
		for (size_t i = 1; i < m_NumRows - 1; ++i)
		{
			for (size_t j = 1; j < m_NumCols - 1; ++j)
			{
				float left = m_Vertices[i * m_NumCols + j - 1].pos.y;
				float right = m_Vertices[i * m_NumCols + j + 1].pos.y;
				float top = m_Vertices[(i - 1) * m_NumCols + j].pos.y;
				float bottom = m_Vertices[(i + 1) * m_NumCols + j].pos.y;
				m_Vertices[i * m_NumCols + j].normal = XMFLOAT3(-right + left, 2.0f * m_SpatialStep, bottom - top);
				XMVECTOR nVec = XMVector3Normalize(XMLoadFloat3(&m_Vertices[i * m_NumCols + j].normal));
				XMStoreFloat3(&m_Vertices[i * m_NumCols + j].normal, nVec);
			}
		}
	}
}

void CpuWavesRender::Disturb(UINT i, UINT j, float magnitude)
{
	// 不要对边界处激起波浪
	assert(i > 1 && i < m_NumRows - 2);
	assert(j > 1 && j < m_NumCols - 2);

	float halfMag = 0.5f * magnitude;

	// 对顶点[i][j]及其相邻顶点修改高度值
	size_t curr = i * (size_t)m_NumCols + j;
	m_Vertices[curr].pos.y += magnitude;
	m_Vertices[curr - 1].pos.y += halfMag;
	m_Vertices[curr + 1].pos.y += halfMag;
	m_Vertices[curr - m_NumCols].pos.y += halfMag;
	m_Vertices[curr + m_NumCols].pos.y += halfMag;

	m_isUpdated = true;
}

void CpuWavesRender::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
	// 更新动态顶点缓冲区的数据
	if (m_isUpdated)
	{
		m_isUpdated = false;
		D3D11_MAPPED_SUBRESOURCE mappedData;
		deviceContext->Map(m_pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
		memcpy_s(mappedData.pData, m_VertexCount * sizeof(VertexPosNormalTex),
			m_Vertices.data(), m_VertexCount * sizeof(VertexPosNormalTex));
		deviceContext->Unmap(m_pVertexBuffer.Get(), 0);
	}

	UINT strides[1] = { sizeof(VertexPosNormalTex) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	effect.SetMaterial(m_Material);
	effect.SetTextureDiffuse(m_pTextureDiffuse.Get());
	effect.SetWorldMatrix(XMLoadFloat4x4(&m_WorldMatrix));
	effect.SetTexTransformMatrix(XMMatrixScaling(m_TexU, m_TexV, 1.0f) * XMMatrixTranslationFromVector(XMLoadFloat2(&m_TexOffset)));
	effect.Apply(deviceContext);
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);

}

void CpuWavesRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	// 先清空可能存在的名称
	D3D11SetDebugObjectName(m_pTextureDiffuse.Get(), nullptr);

	D3D11SetDebugObjectName(m_pTextureDiffuse.Get(), name + ".TextureSRV");
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}

HRESULT GpuWavesRender::InitResource(ID3D11Device* device, const std::wstring& texFileName, UINT rows, UINT cols, float texU, float texV, float timeStep, float spatialStep, float waveSpeed, float damping, float flowSpeedX, float flowSpeedY)
{
	// 防止重复初始化造成内存泄漏
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();
	m_pConstantBuffer.Reset();
	m_pTextureDiffuse.Reset();

	m_pPrevSolution.Reset();
	m_pCurrSolution.Reset();
	m_pNextSolution.Reset();

	m_pPrevSolutionSRV.Reset();
	m_pCurrSolutionSRV.Reset();
	m_pNextSolutionSRV.Reset();

	m_pPrevSolutionUAV.Reset();
	m_pCurrSolutionUAV.Reset();
	m_pNextSolutionUAV.Reset();

	m_pWavesUpdateCS.Reset();
	m_pWavesDisturbCS.Reset();

	// rows和cols都需要是16的倍数，以免出现多余的部分被分配到额外的线程组当中。
	if (rows % 16 || cols % 16)
		return E_INVALIDARG;

	// 初始化水波数据
	Init(rows, cols, texU, texV, timeStep, spatialStep, waveSpeed, damping, flowSpeedX, flowSpeedY);

	// 顶点行(列)数 - 1 = 网格行(列)数
	auto meshData = Geometry::CreateTerrain<VertexPosNormalTex, DWORD>(XMFLOAT2((cols - 1) * spatialStep, (rows - 1) * spatialStep),
		XMUINT2(cols - 1, rows - 1));

	HRESULT hr;

	// 创建顶点缓冲区
	hr = CreateVertexBuffer(device, meshData.vertexVec.data(), (UINT)meshData.vertexVec.size() * sizeof(VertexPosNormalTex),
		m_pVertexBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;
	// 创建索引缓冲区
	hr = CreateIndexBuffer(device, meshData.indexVec.data(), (UINT)meshData.indexVec.size() * sizeof(DWORD),
		m_pIndexBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;
	// 创建常量缓冲区
	hr = CreateConstantBuffer(device, nullptr, sizeof m_CBUpdateSettings, m_pConstantBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建计算着色器
	ComPtr<ID3DBlob> blob;
	hr = CreateShaderFromFile(L"HLSL\\WavesUpdate_CS.cso", L"HLSL\\WavesUpdate_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pWavesUpdateCS.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = CreateShaderFromFile(L"HLSL\\WavesDisturb_CS.cso", L"HLSL\\WavesDisturb_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pWavesDisturbCS.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建缓存计算结果的资源
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = cols;
	texDesc.Height = rows;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE |
		D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	// 由于关于位移程度的计算都在GPU中完成，故需要三个GPU资源，其中
	// 两个用于输入，另一个用于输出结果
	hr = device->CreateTexture2D(&texDesc, nullptr, m_pPrevSolution.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateTexture2D(&texDesc, nullptr, m_pCurrSolution.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateTexture2D(&texDesc, nullptr, m_pNextSolution.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建着色器资源视图
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	hr = device->CreateShaderResourceView(m_pPrevSolution.Get(), &srvDesc, m_pPrevSolutionSRV.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateShaderResourceView(m_pCurrSolution.Get(), &srvDesc, m_pCurrSolutionSRV.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateShaderResourceView(m_pNextSolution.Get(), &srvDesc, m_pNextSolutionSRV.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建无序访问视图
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	uavDesc.Texture2D.MipSlice = 0;
	hr = device->CreateUnorderedAccessView(m_pPrevSolution.Get(), &uavDesc, m_pPrevSolutionUAV.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateUnorderedAccessView(m_pCurrSolution.Get(), &uavDesc, m_pCurrSolutionUAV.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateUnorderedAccessView(m_pNextSolution.Get(), &uavDesc, m_pNextSolutionUAV.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 读取纹理
	if (texFileName.size() > 4)
	{
		if (texFileName.substr(texFileName.size() - 3, 3) == L"dds")
		{
			hr = CreateDDSTextureFromFile(device, texFileName.c_str(), nullptr,
				m_pTextureDiffuse.GetAddressOf());
		}
		else
		{
			hr = CreateWICTextureFromFile(device, texFileName.c_str(), nullptr,
				m_pTextureDiffuse.GetAddressOf());
		}
	}
	return hr;
}

void GpuWavesRender::Update(ID3D11DeviceContext* deviceContext, float dt)
{
	m_AccumulateTime += dt;
	m_TexOffset.x += m_FlowSpeedX * dt;
	m_TexOffset.y += m_FlowSpeedY * dt;

	// 仅仅在累积时间大于时间步长时才更新
	if (m_AccumulateTime > m_TimeStep)
	{
		// 更新常量缓冲区
		D3D11_MAPPED_SUBRESOURCE mappedData;
		m_CBUpdateSettings.waveInfo = XMFLOAT4(m_K1, m_K2, m_K3, 0.0f);
		deviceContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
		memcpy_s(mappedData.pData, sizeof m_CBUpdateSettings, &m_CBUpdateSettings, sizeof m_CBUpdateSettings);
		deviceContext->Unmap(m_pConstantBuffer.Get(), 0);
		// 设置计算所需
		deviceContext->CSSetShader(m_pWavesUpdateCS.Get(), nullptr, 0);
		deviceContext->CSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
		ID3D11UnorderedAccessView* pUAVs[3] = { m_pPrevSolutionUAV.Get(), m_pCurrSolutionUAV.Get(), m_pNextSolutionUAV.Get() };
		deviceContext->CSSetUnorderedAccessViews(0, 3, pUAVs, nullptr);
		// 开始调度
		deviceContext->Dispatch(m_NumCols / 16, m_NumRows / 16, 1);

		// 清除绑定
		pUAVs[0] = pUAVs[1] = pUAVs[2] = nullptr;
		deviceContext->CSSetUnorderedAccessViews(0, 3, pUAVs, nullptr);

		//
		// 对缓冲区进行Ping-pong交换以准备下一次更新
		// 上一次模拟的缓冲区不再需要，用作下一次模拟的输出缓冲
		// 当前模拟的缓冲区变成上一次模拟的缓冲区
		// 下一次模拟的缓冲区变换当前模拟的缓冲区
		//
		auto resTemp = m_pPrevSolution;
		m_pPrevSolution = m_pCurrSolution;
		m_pCurrSolution = m_pNextSolution;
		m_pNextSolution = resTemp;

		auto srvTemp = m_pPrevSolutionSRV;
		m_pPrevSolutionSRV = m_pCurrSolutionSRV;
		m_pCurrSolutionSRV = m_pNextSolutionSRV;
		m_pNextSolutionSRV = srvTemp;

		auto uavTemp = m_pPrevSolutionUAV;
		m_pPrevSolutionUAV = m_pCurrSolutionUAV;
		m_pCurrSolutionUAV = m_pNextSolutionUAV;
		m_pNextSolutionUAV = uavTemp;

		m_AccumulateTime = 0.0f;		// 重置时间
	}
}

void GpuWavesRender::Disturb(ID3D11DeviceContext* deviceContext, UINT i, UINT j, float magnitude)
{
	// 更新常量缓冲区
	D3D11_MAPPED_SUBRESOURCE mappedData;
	m_CBUpdateSettings.waveInfo = XMFLOAT4(0.0f, 0.0f, 0.0f, magnitude);
	m_CBUpdateSettings.index = XMINT4(j, i, 0, 0);
	deviceContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
	memcpy_s(mappedData.pData, sizeof m_CBUpdateSettings, &m_CBUpdateSettings, sizeof m_CBUpdateSettings);
	deviceContext->Unmap(m_pConstantBuffer.Get(), 0);
	// 设置计算所需
	deviceContext->CSSetShader(m_pWavesDisturbCS.Get(), nullptr, 0);
	ID3D11UnorderedAccessView* m_UAVs[1] = { m_pCurrSolutionUAV.Get() };
	deviceContext->CSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	deviceContext->CSSetUnorderedAccessViews(2, 1, m_UAVs, nullptr);

	deviceContext->Dispatch(1, 1, 1);

	// 清除绑定
	m_UAVs[0] = nullptr;
	deviceContext->CSSetUnorderedAccessViews(2, 1, m_UAVs, nullptr);
}

void GpuWavesRender::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
	UINT strides[1] = { sizeof(VertexPosNormalTex) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	effect.SetWavesStates(true, 1.0f / m_NumCols, 1.0f / m_NumCols, m_SpatialStep);	// 开启波浪绘制
	effect.SetMaterial(m_Material);
	effect.SetTextureDiffuse(m_pTextureDiffuse.Get());
	effect.SetTextureDisplacement(m_pCurrSolutionSRV.Get());	// 需要额外设置位移贴图
	effect.SetWorldMatrix(XMLoadFloat4x4(&m_WorldMatrix));
	effect.SetTexTransformMatrix(XMMatrixScaling(m_TexU, m_TexV, 1.0f) * XMMatrixTranslationFromVector(XMLoadFloat2(&m_TexOffset)));
	effect.Apply(deviceContext);
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);

	effect.SetTextureDisplacement(nullptr);	// 解除占用
	effect.SetWavesStates(false);	// 关闭波浪绘制
	effect.Apply(deviceContext);
}

void GpuWavesRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	// 先清空可能存在的名称
	D3D11SetDebugObjectName(m_pTextureDiffuse.Get(), nullptr);

	D3D11SetDebugObjectName(m_pTextureDiffuse.Get(), name + ".TextureSRV");
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
	D3D11SetDebugObjectName(m_pConstantBuffer.Get(), name + ".ConstantBuffer");
	// 由于三个纹理轮流替换使用，故不设置它们的名称
	D3D11SetDebugObjectName(m_pWavesDisturbCS.Get(), name + ".WavesDisturb_CS");
	D3D11SetDebugObjectName(m_pWavesUpdateCS.Get(), name + ".WavesUpdate_CS");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}
