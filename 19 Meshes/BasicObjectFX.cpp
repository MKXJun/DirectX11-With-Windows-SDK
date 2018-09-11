#include "BasicObjectFX.h"
#include <filesystem>

using namespace DirectX;
using namespace std::experimental;

namespace
{
	BasicObjectFX * pInstance = nullptr;
}


BasicObjectFX::BasicObjectFX()
{
	if (pInstance)
		throw std::exception("BasicObjectFX is a singleton!");
	pInstance = this;
}

BasicObjectFX & BasicObjectFX::Get()
{
	if (!pInstance)
		throw std::exception("BasicObjectFX needs an instance!");
	return *pInstance;
}

bool BasicObjectFX::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	ComPtr<ID3DBlob> blob;

	// 创建顶点着色器
	HR(CreateShaderFromFile(L"HLSL\\BasicObject_VS.vso", L"HLSL\\BasicObject_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mBasicObjectVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), mVertexLayout.GetAddressOf()));

	// 创建像素着色器
	HR(CreateShaderFromFile(L"HLSL\\BasicObject_PS.pso", L"HLSL\\BasicObject_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, mBasicObjectPS.GetAddressOf()));

	RenderStates::InitAll(device);
	device->GetImmediateContext(md3dImmediateContext.GetAddressOf());

	// ******************
	// 设置常量缓冲区描述
	mConstantBuffers.assign(4, nullptr);
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;

	cbd.ByteWidth = Align16Bytes(sizeof(CBChangesEveryDrawing));
	HR(device->CreateBuffer(&cbd, nullptr, mConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = Align16Bytes(sizeof(CBChangesEveryFrame));
	HR(device->CreateBuffer(&cbd, nullptr, mConstantBuffers[1].GetAddressOf()));
	cbd.ByteWidth = Align16Bytes(sizeof(CBChangesOnResize));
	HR(device->CreateBuffer(&cbd, nullptr, mConstantBuffers[2].GetAddressOf()));
	cbd.ByteWidth = Align16Bytes(sizeof(CBNeverChange));
	HR(device->CreateBuffer(&cbd, nullptr, mConstantBuffers[3].GetAddressOf()));

	// 预先绑定各自所需的缓冲区，其中每帧更新的缓冲区需要绑定到两个缓冲区上
	md3dImmediateContext->VSSetConstantBuffers(0, 1, mConstantBuffers[0].GetAddressOf());
	md3dImmediateContext->VSSetConstantBuffers(1, 1, mConstantBuffers[1].GetAddressOf());
	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers[2].GetAddressOf());
	md3dImmediateContext->VSSetConstantBuffers(3, 1, mConstantBuffers[3].GetAddressOf());

	md3dImmediateContext->PSSetConstantBuffers(0, 1, mConstantBuffers[0].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(1, 1, mConstantBuffers[1].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(3, 1, mConstantBuffers[3].GetAddressOf());
	// 设置默认图元
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	return true;
}

bool BasicObjectFX::IsInit() const
{
	return md3dImmediateContext != nullptr;
}

void BasicObjectFX::SetRenderDefault()
{
	md3dImmediateContext->IASetInputLayout(mVertexLayout.Get());
	md3dImmediateContext->VSSetShader(mBasicObjectVS.Get(), nullptr, 0);
	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->PSSetShader(mBasicObjectPS.Get(), nullptr, 0);
	md3dImmediateContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	md3dImmediateContext->OMSetDepthStencilState(nullptr, 0);
	md3dImmediateContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}



HRESULT BasicObjectFX::CreateShaderFromFile(const WCHAR * objFileNameInOut, const WCHAR * hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob ** ppBlobOut)
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
void BasicObjectFX::UpdateConstantBuffer(const T& cbuffer)
{
}

template<>
void BasicObjectFX::UpdateConstantBuffer<CBChangesEveryDrawing>(const CBChangesEveryDrawing& cbuffer)
{
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[0].Get(), 0, nullptr, &cbuffer, 0, 0);
}

template<>
void BasicObjectFX::UpdateConstantBuffer<CBChangesEveryFrame>(const CBChangesEveryFrame& cbuffer)
{
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[1].Get(), 0, nullptr, &cbuffer, 0, 0);
}

template<>
void BasicObjectFX::UpdateConstantBuffer<CBChangesOnResize>(const CBChangesOnResize& cbuffer)
{
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[2].Get(), 0, nullptr, &cbuffer, 0, 0);
}

template<>
void BasicObjectFX::UpdateConstantBuffer<CBNeverChange>(const CBNeverChange& cbuffer)
{
	md3dImmediateContext->UpdateSubresource(mConstantBuffers[3].Get(), 0, nullptr, &cbuffer, 0, 0);
}
