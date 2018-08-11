#include "BasicFX.h"
#include <filesystem>

using namespace DirectX;
using namespace std::experimental;


bool BasicFX::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	const D3D11_SO_DECLARATION_ENTRY posColorLayout[2] = {
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "COLOR", 0, 0, 4, 0 }
	};

	const D3D11_SO_DECLARATION_ENTRY posNormalColorLayout[3] = {
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "NORMAL", 0, 0, 3, 0 },
		{ 0, "COLOR", 0, 0, 4, 0 }
	};

	UINT stridePosColor = sizeof(VertexPosColor);
	UINT stridePosNormalColor = sizeof(VertexPosNormalColor);

	ComPtr<ID3DBlob> blob;

	//
	// 流输出分裂三角形
	//
	HR(CreateShaderFromFile(L"HLSL\\TriangleSO_VS.vso", L"HLSL\\TriangleSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mTriangleSOVS.GetAddressOf()));
	// 创建顶点输入布局
	HR(device->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), mVertexPosColorLayout.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\TriangleSO_GS.gso", L"HLSL\\TriangleSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posColorLayout, ARRAYSIZE(posColorLayout),
		&stridePosColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, mTriangleSOGS.GetAddressOf()));
	
	//
	// 绘制分形三角形
	//
	HR(CreateShaderFromFile(L"HLSL\\Triangle_VS.vso", L"HLSL\\Triangle_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mTriangleVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Triangle_PS.pso", L"HLSL\\Triangle_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mTrianglePS.GetAddressOf()));


	//
	// 流输出分形球体
	//
	HR(CreateShaderFromFile(L"HLSL\\SphereSO_VS.vso", L"HLSL\\SphereSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mSphereSOVS.GetAddressOf()));
	// 创建顶点输入布局
	HR(device->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), mVertexPosNormalColorLayout.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\SphereSO_GS.gso", L"HLSL\\SphereSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posNormalColorLayout, ARRAYSIZE(posNormalColorLayout),
		&stridePosNormalColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, mSphereSOGS.GetAddressOf()));
	
	//
	// 绘制球体
	//
	HR(CreateShaderFromFile(L"HLSL\\Sphere_VS.vso", L"HLSL\\Sphere_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mSphereVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Sphere_PS.pso", L"HLSL\\Sphere_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mSpherePS.GetAddressOf()));
	

	//
	// 流输出分形雪花
	//
	HR(CreateShaderFromFile(L"HLSL\\SnowSO_VS.vso", L"HLSL\\SnowSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mSnowSOVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\SnowSO_GS.gso", L"HLSL\\SnowSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posColorLayout, ARRAYSIZE(posColorLayout),
		&stridePosColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, mSnowSOGS.GetAddressOf()));

	//
	// 绘制雪花
	//
	HR(CreateShaderFromFile(L"HLSL\\Snow_VS.vso", L"HLSL\\Snow_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mSnowVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Snow_PS.pso", L"HLSL\\Snow_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mSnowPS.GetAddressOf()));


	//
	// 绘制法向量
	//
	HR(CreateShaderFromFile(L"HLSL\\Normal_VS.vso", L"HLSL\\Normal_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mNormalVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Normal_GS.gso", L"HLSL\\Normal_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mNormalGS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Normal_PS.pso", L"HLSL\\Normal_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mNormalPS.GetAddressOf()));

	

	RenderStates::InitAll(device);
	device->GetImmediateContext(md3dImmediateContext.GetAddressOf());

	// ******************
	// 设置常量缓冲区描述
	mConstantBuffers.assign(3, nullptr);
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;

	cbd.ByteWidth = Align16Bytes(sizeof(CBChangesEveryFrame));
	HR(device->CreateBuffer(&cbd, nullptr, mConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = Align16Bytes(sizeof(CBChangesOnResize));
	HR(device->CreateBuffer(&cbd, nullptr, mConstantBuffers[1].GetAddressOf()));
	cbd.ByteWidth = Align16Bytes(sizeof(CBNeverChange));
	HR(device->CreateBuffer(&cbd, nullptr, mConstantBuffers[2].GetAddressOf()));

	// 预先绑定各自所需的缓冲区
	md3dImmediateContext->VSSetConstantBuffers(0, 1, mConstantBuffers[0].GetAddressOf());
	md3dImmediateContext->VSSetConstantBuffers(1, 1, mConstantBuffers[1].GetAddressOf());
	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers[2].GetAddressOf());

	md3dImmediateContext->GSSetConstantBuffers(0, 1, mConstantBuffers[0].GetAddressOf());
	md3dImmediateContext->GSSetConstantBuffers(1, 1, mConstantBuffers[1].GetAddressOf());
	md3dImmediateContext->GSSetConstantBuffers(2, 1, mConstantBuffers[2].GetAddressOf());

	md3dImmediateContext->PSSetConstantBuffers(2, 1, mConstantBuffers[2].GetAddressOf());
	return true;
}

bool BasicFX::IsInit() const
{
	return md3dImmediateContext != nullptr;
}

void BasicFX::SetRenderSplitedTriangle()
{
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dImmediateContext->IASetInputLayout(mVertexPosColorLayout.Get());
	md3dImmediateContext->VSSetShader(mTriangleVS.Get(), nullptr, 0);
	// 关闭流输出
	md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	md3dImmediateContext->SOSetTargets(1, bufferArray, &offset);
	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->PSSetShader(mTrianglePS.Get(), nullptr, 0);
}

void BasicFX::SetRenderSplitedSnow()
{
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	md3dImmediateContext->IASetInputLayout(mVertexPosColorLayout.Get());
	md3dImmediateContext->VSSetShader(mSnowVS.Get(), nullptr, 0);
	// 关闭流输出
	md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	md3dImmediateContext->SOSetTargets(1, bufferArray, &offset);
	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->PSSetShader(mSnowPS.Get(), nullptr, 0);
}

void BasicFX::SetRenderSplitedSphere()
{
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dImmediateContext->IASetInputLayout(mVertexPosNormalColorLayout.Get());
	md3dImmediateContext->VSSetShader(mSphereVS.Get(), nullptr, 0);
	// 关闭流输出
	md3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	md3dImmediateContext->SOSetTargets(1, bufferArray, &offset);
	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->PSSetShader(mSpherePS.Get(), nullptr, 0);
}


void BasicFX::SetStreamOutputSplitedTriangle(ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	md3dImmediateContext->SOSetTargets(1, &nullBuffer, &offset);

	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dImmediateContext->IASetInputLayout(mVertexPosColorLayout.Get());

	md3dImmediateContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	md3dImmediateContext->VSSetShader(mTriangleSOVS.Get(), nullptr, 0);
	md3dImmediateContext->GSSetShader(mTriangleSOGS.Get(), nullptr, 0);

	md3dImmediateContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);
;
	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->PSSetShader(nullptr, nullptr, 0);
}

void BasicFX::SetStreamOutputSplitedSnow(ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	md3dImmediateContext->SOSetTargets(1, &nullBuffer, &offset);

	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	md3dImmediateContext->IASetInputLayout(mVertexPosColorLayout.Get());
	md3dImmediateContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	md3dImmediateContext->VSSetShader(mSnowSOVS.Get(), nullptr, 0);
	md3dImmediateContext->GSSetShader(mSnowSOGS.Get(), nullptr, 0);

	md3dImmediateContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);

	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->PSSetShader(nullptr, nullptr, 0);
}

void BasicFX::SetStreamOutputSplitedSphere(ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosNormalColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	md3dImmediateContext->SOSetTargets(1, &nullBuffer, &offset);

	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dImmediateContext->IASetInputLayout(mVertexPosNormalColorLayout.Get());
	md3dImmediateContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	md3dImmediateContext->VSSetShader(mSphereSOVS.Get(), nullptr, 0);
	md3dImmediateContext->GSSetShader(mSphereSOGS.Get(), nullptr, 0);

	md3dImmediateContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);

	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->PSSetShader(nullptr, nullptr, 0);
}

void BasicFX::SetRenderNormal()
{
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	md3dImmediateContext->IASetInputLayout(mVertexPosNormalColorLayout.Get());
	md3dImmediateContext->VSSetShader(mNormalVS.Get(), nullptr, 0);
	md3dImmediateContext->GSSetShader(mNormalGS.Get(), nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	md3dImmediateContext->SOSetTargets(1, bufferArray, &offset);
	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->PSSetShader(mNormalPS.Get(), nullptr, 0);
}



HRESULT BasicFX::CreateShaderFromFile(const WCHAR * objFileNameInOut, const WCHAR * hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob ** ppBlobOut)
{
	HRESULT hr = S_OK;

	// 寻找是否有已经编译好的顶点着色器
	if (objFileNameInOut && filesystem::exists(objFileNameInOut))
	{
		HR(D3DReadFileToBlob(objFileNameInOut, ppBlobOut));
	}
	else
	{
		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		// 设置 D3DCOMPILE_DEBUG 标志用于获取着色器调试信息。该标志可以提升调试体验，
		// 但仍然允许着色器进行优化操作
		dwShaderFlags |= D3DCOMPILE_DEBUG;

		// 在Debug环境下禁用优化以避免出现一些不合理的情况
		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ComPtr<ID3DBlob> errorBlob = nullptr;
		hr = D3DCompileFromFile(hlslFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, shaderModel,
			dwShaderFlags, 0, ppBlobOut, errorBlob.GetAddressOf());
		if (FAILED(hr))
		{
			if (errorBlob != nullptr)
			{
				OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			}
			return hr;
		}

		// 若指定了输出文件名，则将着色器二进制信息输出
		if (objFileNameInOut)
		{
			HR(D3DWriteBlobToFile(*ppBlobOut, objFileNameInOut, FALSE));
		}
	}

	return hr;
}

template<class T>
void BasicFX::UpdateConstantBuffer(const T& cbuffer)
{
}

template<>
void BasicFX::UpdateConstantBuffer<CBChangesEveryFrame>(const CBChangesEveryFrame& cbuffer)
{
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[0].Get(), 0, nullptr, &cbuffer, 0, 0);
}

template<>
void BasicFX::UpdateConstantBuffer<CBChangesOnResize>(const CBChangesOnResize& cbuffer)
{
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[1].Get(), 0, nullptr, &cbuffer, 0, 0);
}

template<>
void BasicFX::UpdateConstantBuffer<CBNeverChange>(const CBNeverChange& cbuffer)
{
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[2].Get(), 0, nullptr, &cbuffer, 0, 0);
}
