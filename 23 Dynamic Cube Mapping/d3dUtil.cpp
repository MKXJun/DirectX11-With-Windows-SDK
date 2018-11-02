#include "d3dUtil.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace std::experimental;

HRESULT CreateShaderFromFile(const WCHAR * objFileNameInOut, const WCHAR * hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob ** ppBlobOut)
{
	using namespace Microsoft::WRL;

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

ComPtr<ID3D11ShaderResourceView> CreateDDSTexture2DArrayFromFile(
	ComPtr<ID3D11Device> device,
	ComPtr<ID3D11DeviceContext> deviceContext,
	const std::vector<std::wstring>& filenames,
	UINT maxMipMapSize)
{
	// 检查设备与设备上下文是否非空
	if (!device || !deviceContext)
		return nullptr;

	// ******************
	// 1. 读取所有纹理
	//
	UINT size = (UINT)filenames.size();
	UINT mipLevel = maxMipMapSize;
	std::vector<ComPtr<ID3D11Texture2D>> srcTex(size);
	UINT width, height;
	DXGI_FORMAT format;
	for (size_t i = 0; i < size; ++i)
	{
		// 由于这些纹理并不会被GPU使用，我们使用D3D11_USAGE_STAGING枚举值
		// 使得CPU可以读取资源
		HR(CreateDDSTextureFromFileEx(device.Get(),
			deviceContext.Get(),
			filenames[i].c_str(),
			maxMipMapSize,
			D3D11_USAGE_STAGING,							// Usage
			0,												// BindFlags
			D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,	// CpuAccessFlags
			0,												// MiscFlags
			false,
			(ID3D11Resource**)srcTex[i].GetAddressOf(),
			nullptr));

		// 读取创建好的纹理Mipmap等级, 宽度和高度
		D3D11_TEXTURE2D_DESC texDesc;
		srcTex[i]->GetDesc(&texDesc);
		if (i == 0)
		{
			mipLevel = texDesc.MipLevels;
			width = texDesc.Width;
			height = texDesc.Height;
			format = texDesc.Format;
		}
		// 这里断言所有纹理的MipMap等级，宽度和高度应当一致
		assert(mipLevel == texDesc.MipLevels);
		assert(texDesc.Width == width && texDesc.Height == height);
		// 这里要求所有提供的图片数据格式应当是一致的，若存在不一致的情况，请
		// 使用dxtex.exe(DirectX Texture Tool)将所有的图片转成一致的数据格式
		assert(texDesc.Format == format);

	}

	// ******************
	// 2.创建纹理数组
	//
	D3D11_TEXTURE2D_DESC texDesc, texArrayDesc;
	srcTex[0]->GetDesc(&texDesc);
	texArrayDesc.Width = texDesc.Width;
	texArrayDesc.Height = texDesc.Height;
	texArrayDesc.MipLevels = texDesc.MipLevels;
	texArrayDesc.ArraySize = size;
	texArrayDesc.Format = texDesc.Format;
	texArrayDesc.SampleDesc.Count = 1;		// 不能使用多重采样
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;
	texArrayDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> texArray;
	HR(device->CreateTexture2D(&texArrayDesc, nullptr, texArray.GetAddressOf()));

	// ******************
	// 3.将所有的纹理子资源赋值到纹理数组中
	//

	// 每个纹理元素
	for (UINT i = 0; i < size; ++i)
	{
		// 纹理中的每个mipmap等级
		for (UINT j = 0; j < mipLevel; ++j)
		{
			D3D11_MAPPED_SUBRESOURCE mappedTex2D;
			// 允许映射索引i纹理中，索引j的mipmap等级的2D纹理
			HR(deviceContext->Map(srcTex[i].Get(),
				j, D3D11_MAP_READ, 0, &mappedTex2D));

			deviceContext->UpdateSubresource(
				texArray.Get(),
				D3D11CalcSubresource(j, i, mipLevel),	// i * mipLevel + j
				nullptr,
				mappedTex2D.pData,
				mappedTex2D.RowPitch,
				mappedTex2D.DepthPitch);
			// 停止映射
			deviceContext->Unmap(srcTex[i].Get(), j);
		}
	}

	// ******************
	// 4.创建纹理数组的SRV
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texArrayDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesc.Texture2DArray.MostDetailedMip = 0;
	viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
	viewDesc.Texture2DArray.FirstArraySlice = 0;
	viewDesc.Texture2DArray.ArraySize = size;

	ComPtr<ID3D11ShaderResourceView> texArraySRV;
	HR(device->CreateShaderResourceView(texArray.Get(), &viewDesc, texArraySRV.GetAddressOf()));


	// 已经确保所有资源由ComPtr管理，无需手动释放

	return texArraySRV;
}

ComPtr<ID3D11ShaderResourceView> CreateWICTextureCubeFromFile(
	ComPtr<ID3D11Device> device, 
	ComPtr<ID3D11DeviceContext> deviceContext, 
	std::wstring cubemapFileName,
	bool generateMips)
{
	// 检查设备与设备上下文是否非空
	if (!device || !deviceContext)
		return nullptr;

	// ******************
	// 1.读取天空盒纹理
	//

	ComPtr<ID3D11Texture2D> srcTex;
	ComPtr<ID3D11ShaderResourceView> srcTexSRV;

	// 该资源用于GPU复制
	HR(CreateWICTextureFromFileEx(device.Get(),
		deviceContext.Get(),
		cubemapFileName.c_str(),
		0,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | (generateMips ? D3D11_BIND_RENDER_TARGET : 0),
		0,
		(generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0),
		WIC_LOADER_DEFAULT,
		(ID3D11Resource**)srcTex.GetAddressOf(),
		(generateMips ? srcTexSRV.GetAddressOf() : nullptr)));
	// (可选)生成mipmap链
	if (generateMips)
	{
		deviceContext->GenerateMips(srcTexSRV.Get());
	}

	
	// ******************
	// 2.创建包含6个纹理的数组
	//

	D3D11_TEXTURE2D_DESC texDesc, texCubeDesc;
	srcTex->GetDesc(&texDesc);
	
	// 确保宽高比4:3
	assert(texDesc.Width * 3 == texDesc.Height * 4);

	UINT squareLength = texDesc.Width / 4;

	texCubeDesc.Width = squareLength;
	texCubeDesc.Height = squareLength;
	// 例如64x48的天空盒，可以产生7级mipmap链，但天空盒的每个面是16x16，对应5级mipmap链，因此需要减2
	texCubeDesc.MipLevels = (generateMips ? texDesc.MipLevels - 2 : 1);
	texCubeDesc.ArraySize = 6;
	texCubeDesc.Format = texDesc.Format;
	texCubeDesc.SampleDesc.Count = 1;
	texCubeDesc.SampleDesc.Quality = 0;
	texCubeDesc.Usage = D3D11_USAGE_DEFAULT;
	texCubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texCubeDesc.CPUAccessFlags = 0;
	texCubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;	// 标记为TextureCube

	ComPtr<ID3D11Texture2D> texCube;
	HR(device->CreateTexture2D(&texCubeDesc, nullptr, texCube.GetAddressOf()));

	// ******************
	// 3.选取原天空盒纹理的6个子正方形区域，拷贝到该数组中
	//
	
	D3D11_BOX box;
	// box坐标轴如下: 
	//    front
	//   / 
	//  /_____right
	//  |
	//  |
	//  bottom
	box.front = 0;
	box.back = 1;

	for (UINT i = 0; i < texCubeDesc.MipLevels; ++i)
	{
		// +X面拷贝
		box.left = squareLength * 2;
		box.top = squareLength;
		box.right = squareLength * 3;
		box.bottom = squareLength * 2;
		deviceContext->CopySubresourceRegion(
			texCube.Get(),
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_POSITIVE_X, texCubeDesc.MipLevels),
			0, 0, 0,
			srcTex.Get(),
			i,
			&box);

		// -X面拷贝
		box.left = 0;
		box.top = squareLength;
		box.right = squareLength;
		box.bottom = squareLength * 2;
		deviceContext->CopySubresourceRegion(
			texCube.Get(),
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_NEGATIVE_X, texCubeDesc.MipLevels),
			0, 0, 0,
			srcTex.Get(),
			i,
			&box);

		// +Y面拷贝
		box.left = squareLength;
		box.top = 0;
		box.right = squareLength * 2;
		box.bottom = squareLength;
		deviceContext->CopySubresourceRegion(
			texCube.Get(),
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_POSITIVE_Y, texCubeDesc.MipLevels),
			0, 0, 0,
			srcTex.Get(),
			i,
			&box);


		// -Y面拷贝
		box.left = squareLength;
		box.top = squareLength * 2;
		box.right = squareLength * 2;
		box.bottom = squareLength * 3;
		deviceContext->CopySubresourceRegion(
			texCube.Get(),
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_NEGATIVE_Y, texCubeDesc.MipLevels),
			0, 0, 0,
			srcTex.Get(),
			i,
			&box);

		// +Z面拷贝
		box.left = squareLength;
		box.top = squareLength;
		box.right = squareLength * 2;
		box.bottom = squareLength * 2;
		deviceContext->CopySubresourceRegion(
			texCube.Get(),
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_POSITIVE_Z, texCubeDesc.MipLevels),
			0, 0, 0,
			srcTex.Get(),
			i,
			&box);

		// -Z面拷贝
		box.left = squareLength * 3;
		box.top = squareLength;
		box.right = squareLength * 4;
		box.bottom = squareLength * 2;
		deviceContext->CopySubresourceRegion(
			texCube.Get(),
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_NEGATIVE_Z, texCubeDesc.MipLevels),
			0, 0, 0,
			srcTex.Get(),
			i,
			&box);

		// 下一个mipLevel的纹理宽高都是原来的1/2
		squareLength /= 2;
	}
	

	// ******************
	// 4.创建立方体纹理的SRV
	//

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texCubeDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	viewDesc.TextureCube.MostDetailedMip = 0;
	viewDesc.TextureCube.MipLevels = texCubeDesc.MipLevels;

	ComPtr<ID3D11ShaderResourceView> texCubeSRV;
	HR(device->CreateShaderResourceView(texCube.Get(), &viewDesc, texCubeSRV.GetAddressOf()));

	return texCubeSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateWICTextureCubeFromFile(
	Microsoft::WRL::ComPtr<ID3D11Device> device, 
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext, 
	std::vector<std::wstring> cubemapFileNames, 
	bool generateMips)
{
	// 检查设备与设备上下文是否非空
	if (!device || !deviceContext)
		return nullptr;

	// 立方体纹理需要6个正方形贴图文件
	assert(cubemapFileNames.size() == 6);

	// ******************
	// 1.读取纹理
	//
	
	std::vector<ComPtr<ID3D11Texture2D>> srcTex(6);
	std::vector<ComPtr<ID3D11ShaderResourceView>> srcTexSRV(6);
	UINT width, height;
	DXGI_FORMAT format;

	for (int i = 0; i < 6; ++i)
	{
		// 该资源用于GPU复制
		HR(CreateWICTextureFromFileEx(device.Get(),
			deviceContext.Get(),
			cubemapFileNames[i].c_str(),
			0,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE | (generateMips ? D3D11_BIND_RENDER_TARGET : 0),
			0,
			(generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0),
			WIC_LOADER_DEFAULT,
			(ID3D11Resource**)srcTex[i].GetAddressOf(),
			(generateMips ? srcTexSRV[i].GetAddressOf() : nullptr)));
		// (可选)生成mipmap链
		if (generateMips)
		{
			deviceContext->GenerateMips(srcTexSRV[i].Get());
		}

		D3D11_TEXTURE2D_DESC texDesc;
		srcTex[i]->GetDesc(&texDesc);
		if (i == 0)
		{
			width = texDesc.Width;
			height = texDesc.Height;
			format = texDesc.Format;
		}
		// 这里断言所有纹理的宽度和高度应当一致
		assert(texDesc.Width == width && texDesc.Height == height);
		// 这里要求所有提供的图片数据格式应当是一致的，若存在不一致的情况，请
		// 使用dxtex.exe(DirectX Texture Tool)将所有的图片转成一致的数据格式
		assert(texDesc.Format == format);
	}
	
	// ******************
	// 2.创建包含6个纹理的数组
	//

	D3D11_TEXTURE2D_DESC texDesc, texCubeDesc;
	srcTex[0]->GetDesc(&texDesc);

	texCubeDesc.Width = texDesc.Width;
	texCubeDesc.Height = texDesc.Height;
	texCubeDesc.MipLevels = (generateMips ? texDesc.MipLevels : 1);
	texCubeDesc.ArraySize = 6;
	texCubeDesc.Format = texDesc.Format;
	texCubeDesc.SampleDesc.Count = 1;
	texCubeDesc.SampleDesc.Quality = 0;
	texCubeDesc.Usage = D3D11_USAGE_DEFAULT;
	texCubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texCubeDesc.CPUAccessFlags = 0;
	texCubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;	// 标记为TextureCube

	ComPtr<ID3D11Texture2D> texCube;
	HR(device->CreateTexture2D(&texCubeDesc, nullptr, texCube.GetAddressOf()));

	// ******************
	// 3.选取原位图的6个子正方形区域，拷贝到该数组中
	//
	for (int i = 0; i < 6; ++i)
	{
		for (UINT j = 0; j < texCubeDesc.MipLevels; ++j)
		{
			deviceContext->CopySubresourceRegion(
				texCube.Get(),
				D3D11CalcSubresource(j, i, texCubeDesc.MipLevels),
				0, 0, 0,
				srcTex[i].Get(),
				j,
				nullptr);
		}
	}
	
	// ******************
	// 4.创建立方体纹理的SRV
	//

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texCubeDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	viewDesc.TextureCube.MostDetailedMip = 0;
	viewDesc.TextureCube.MipLevels = texCubeDesc.MipLevels;

	ComPtr<ID3D11ShaderResourceView> texCubeSRV;
	HR(device->CreateShaderResourceView(texCube.Get(), &viewDesc, texCubeSRV.GetAddressOf()));

	return texCubeSRV;
}


