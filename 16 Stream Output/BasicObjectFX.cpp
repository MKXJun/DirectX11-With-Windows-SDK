#include "Effects.h"
#include "EffectHelper.h"
#include "Vertex.h"
#include <d3dcompiler.h>
#include <experimental/filesystem>
using namespace DirectX;
using namespace std::experimental;

//
// 这些结构体对应HLSL的结构体，仅供该文件使用。需要按16字节对齐
//

struct CBChangesEveryFrame
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
};

struct CBChangesOnResize
{
	DirectX::XMMATRIX proj;
};


struct CBChangesRarely
{
	DirectionalLight dirLight[BasicObjectFX::maxLights];
	PointLight pointLight[BasicObjectFX::maxLights];
	SpotLight spotLight[BasicObjectFX::maxLights];
	Material material;
	DirectX::XMMATRIX view;
	DirectX::XMFLOAT3 sphereCenter;
	float sphereRadius;
	DirectX::XMFLOAT3 eyePos;
	float pad;
};


//
// BasicObjectFX::Impl 需要先于BasicObjectFX的定义
//

class BasicObjectFX::Impl : public AlignedType<BasicObjectFX::Impl>
{
public:
	// 必须显式指定
	Impl() = default;
	~Impl() = default;

	// objFileNameInOut为编译好的着色器二进制文件(.*so)，若有指定则优先寻找该文件并读取
	// hlslFileName为着色器代码，若未找到着色器二进制文件则编译着色器代码
	// 编译成功后，若指定了objFileNameInOut，则保存编译好的着色器二进制信息到该文件
	// ppBlobOut输出着色器二进制信息
	HRESULT CreateShaderFromFile(const WCHAR* objFileNameInOut, const WCHAR* hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppBlobOut);

public:
	// 需要16字节对齐的优先放在前面
	CBufferObject<0, CBChangesEveryFrame> cbFrame;		// 每次对象绘制的常量缓冲区
	CBufferObject<1, CBChangesOnResize>   cbOnResize;	// 每次窗口大小变更的常量缓冲区
	CBufferObject<2, CBChangesRarely>     cbRarely;		// 几乎不会变更的常量缓冲区
	BOOL isDirty;										// 是否有值变更
	std::vector<CBufferBase*> cBufferPtrs;				// 统一管理上面所有的常量缓冲区


	ComPtr<ID3D11VertexShader> triangleSOVS;
	ComPtr<ID3D11GeometryShader> triangleSOGS;

	ComPtr<ID3D11VertexShader> triangleVS;
	ComPtr<ID3D11PixelShader> trianglePS;

	ComPtr<ID3D11VertexShader> sphereSOVS;
	ComPtr<ID3D11GeometryShader> sphereSOGS;

	ComPtr<ID3D11VertexShader> sphereVS;
	ComPtr<ID3D11PixelShader> spherePS;

	ComPtr<ID3D11VertexShader> snowSOVS;
	ComPtr<ID3D11GeometryShader> snowSOGS;

	ComPtr<ID3D11VertexShader> snowVS;
	ComPtr<ID3D11PixelShader> snowPS;

	ComPtr<ID3D11VertexShader> normalVS;
	ComPtr<ID3D11GeometryShader> normalGS;
	ComPtr<ID3D11PixelShader> normalPS;

	ComPtr<ID3D11InputLayout> vertexPosColorLayout;			// VertexPosColor输入布局
	ComPtr<ID3D11InputLayout> vertexPosNormalColorLayout;	// VertexPosNormalColor输入布局

	ComPtr<ID3D11ShaderResourceView> texture;				// 用于绘制的纹理

};

//
// BasicObjectFX
//

namespace
{
	// BasicObjectFX单例
	static BasicObjectFX * pInstance = nullptr;
}

BasicObjectFX::BasicObjectFX()
{
	if (pInstance)
		throw std::exception("BasicObjectFX is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<BasicObjectFX::Impl>();
}

BasicObjectFX::~BasicObjectFX()
{
}

BasicObjectFX::BasicObjectFX(BasicObjectFX && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

BasicObjectFX & BasicObjectFX::operator=(BasicObjectFX && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
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

	if (!pImpl->cBufferPtrs.empty())
		return true;


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

	// ******************
	// 流输出分裂三角形
	//
	HR(pImpl->CreateShaderFromFile(L"HLSL\\TriangleSO_VS.vso", L"HLSL\\TriangleSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->triangleSOVS.GetAddressOf()));
	// 创建顶点输入布局
	HR(device->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), pImpl->vertexPosColorLayout.GetAddressOf()));
	HR(pImpl->CreateShaderFromFile(L"HLSL\\TriangleSO_GS.gso", L"HLSL\\TriangleSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posColorLayout, ARRAYSIZE(posColorLayout),
		&stridePosColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, pImpl->triangleSOGS.GetAddressOf()));

	// ******************
	// 绘制分形三角形
	//
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Triangle_VS.vso", L"HLSL\\Triangle_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->triangleVS.GetAddressOf()));
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Triangle_PS.pso", L"HLSL\\Triangle_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->trianglePS.GetAddressOf()));


	// ******************
	// 流输出分形球体
	//
	HR(pImpl->CreateShaderFromFile(L"HLSL\\SphereSO_VS.vso", L"HLSL\\SphereSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->sphereSOVS.GetAddressOf()));
	// 创建顶点输入布局
	HR(device->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), pImpl->vertexPosNormalColorLayout.GetAddressOf()));
	HR(pImpl->CreateShaderFromFile(L"HLSL\\SphereSO_GS.gso", L"HLSL\\SphereSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posNormalColorLayout, ARRAYSIZE(posNormalColorLayout),
		&stridePosNormalColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, pImpl->sphereSOGS.GetAddressOf()));

	// ******************
	// 绘制球体
	//
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Sphere_VS.vso", L"HLSL\\Sphere_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->sphereVS.GetAddressOf()));
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Sphere_PS.pso", L"HLSL\\Sphere_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->spherePS.GetAddressOf()));


	// ******************
	// 流输出分形雪花
	//
	HR(pImpl->CreateShaderFromFile(L"HLSL\\SnowSO_VS.vso", L"HLSL\\SnowSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->snowSOVS.GetAddressOf()));
	HR(pImpl->CreateShaderFromFile(L"HLSL\\SnowSO_GS.gso", L"HLSL\\SnowSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posColorLayout, ARRAYSIZE(posColorLayout),
		&stridePosColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, pImpl->snowSOGS.GetAddressOf()));

	// ******************
	// 绘制雪花
	//
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Snow_VS.vso", L"HLSL\\Snow_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->snowVS.GetAddressOf()));
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Snow_PS.pso", L"HLSL\\Snow_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->snowPS.GetAddressOf()));


	// ******************
	// 绘制法向量
	//
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Normal_VS.vso", L"HLSL\\Normal_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalVS.GetAddressOf()));
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Normal_GS.gso", L"HLSL\\Normal_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalGS.GetAddressOf()));
	HR(pImpl->CreateShaderFromFile(L"HLSL\\Normal_PS.pso", L"HLSL\\Normal_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalPS.GetAddressOf()));


	// 初始化
	RenderStates::InitAll(device);

	pImpl->cBufferPtrs.assign({
		&pImpl->cbFrame, 
		&pImpl->cbOnResize, 
		&pImpl->cbRarely});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		pBuffer->CreateBuffer(device);
	}

	return true;
}

void BasicObjectFX::SetRenderSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());
	deviceContext->VSSetShader(pImpl->triangleVS.Get(), nullptr, 0);
	// 关闭流输出
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	deviceContext->SOSetTargets(1, bufferArray, &offset);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->trianglePS.Get(), nullptr, 0);
}

void BasicObjectFX::SetRenderSplitedSnow(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());
	deviceContext->VSSetShader(pImpl->snowVS.Get(), nullptr, 0);
	// 关闭流输出
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	deviceContext->SOSetTargets(1, bufferArray, &offset);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->snowPS.Get(), nullptr, 0);
}

void BasicObjectFX::SetRenderSplitedSphere(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalColorLayout.Get());
	deviceContext->VSSetShader(pImpl->sphereVS.Get(), nullptr, 0);
	// 关闭流输出
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	deviceContext->SOSetTargets(1, bufferArray, &offset);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->spherePS.Get(), nullptr, 0);

}

void BasicObjectFX::SetStreamOutputSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	deviceContext->SOSetTargets(1, &nullBuffer, &offset);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());

	deviceContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	deviceContext->VSSetShader(pImpl->triangleSOVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->triangleSOGS.Get(), nullptr, 0);

	deviceContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);
	;
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(nullptr, nullptr, 0);

}

void BasicObjectFX::SetStreamOutputSplitedSnow(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	deviceContext->SOSetTargets(1, &nullBuffer, &offset);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());
	deviceContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	deviceContext->VSSetShader(pImpl->snowSOVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->snowSOGS.Get(), nullptr, 0);

	deviceContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);

	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(nullptr, nullptr, 0);

}

void BasicObjectFX::SetStreamOutputSplitedSphere(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosNormalColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	deviceContext->SOSetTargets(1, &nullBuffer, &offset);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalColorLayout.Get());
	deviceContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	deviceContext->VSSetShader(pImpl->sphereSOVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->sphereSOGS.Get(), nullptr, 0);

	deviceContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);

	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(nullptr, nullptr, 0);

}

void BasicObjectFX::SetRenderNormal(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalColorLayout.Get());
	deviceContext->VSSetShader(pImpl->normalVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->normalGS.Get(), nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	deviceContext->SOSetTargets(1, bufferArray, &offset);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->normalPS.Get(), nullptr, 0);

}




void XM_CALLCONV BasicObjectFX::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixInverse(nullptr, W);	// 两次转置抵消
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->cbOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetWorldViewProjMatrix(FXMMATRIX W, CXMMATRIX V, CXMMATRIX P)
{
	pImpl->cbFrame.data.world = XMMatrixTranspose(W);
	pImpl->cbFrame.data.worldInvTranspose = XMMatrixInverse(nullptr, W);	// 两次转置抵消
	pImpl->cbRarely.data.view = XMMatrixTranspose(V);
	pImpl->cbOnResize.data.proj = XMMatrixTranspose(P);

	auto& pCBuffers = pImpl->cBufferPtrs;
	pCBuffers[0]->isDirty = pCBuffers[1]->isDirty = pCBuffers[2]->isDirty = true;
	pImpl->isDirty = true;
}

void BasicObjectFX::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.material = material;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicObjectFX::SetEyePos(FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->cbRarely;
	XMStoreFloat3(&cBuffer.data.eyePos, eyePos);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetSphereCenter(const DirectX::XMFLOAT3 & center)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.sphereCenter = center;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::SetSphereRadius(float radius)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.sphereRadius = radius;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicObjectFX::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	pCBuffers[2]->BindVS(deviceContext);

	pCBuffers[0]->BindGS(deviceContext);
	pCBuffers[1]->BindGS(deviceContext);
	pCBuffers[2]->BindGS(deviceContext);
	
	pCBuffers[2]->BindPS(deviceContext);

	// 设置纹理
	deviceContext->PSSetShaderResources(0, 1, pImpl->texture.GetAddressOf());

	if (pImpl->isDirty)
	{
		pImpl->isDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}

//
// BasicObjectFX::Impl实现部分
//


HRESULT BasicObjectFX::Impl::CreateShaderFromFile(const WCHAR * objFileNameInOut, const WCHAR * hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob ** ppBlobOut)
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


