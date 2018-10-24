//
// 该头文件需要在包含特效类实现的源文件中使用，且必须晚于Effects.h和d3dUtil.h包含
// 

#ifndef EFFECTHELPER_H
#define EFFECTHELPER_H


// 若类需要内存对齐，从该类派生
template<class DerivedType>
struct AlignedType
{
	static void* operator new(size_t size)
	{
		const size_t alignedSize = __alignof(DerivedType);

		static_assert(alignedSize > 8, "AlignedNew is only useful for types with > 8 byte alignment! Did you forget a __declspec(align) on DerivedType?");

		void* ptr = _aligned_malloc(size, alignedSize);

		if (!ptr)
			throw std::bad_alloc();

		return ptr;
	}

	static void operator delete(void * ptr)
	{
		_aligned_free(ptr);
	}
};

struct CBufferBase
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	bool isDirty;
	ComPtr<ID3D11Buffer> cBuffer;

	virtual void CreateBuffer(ComPtr<ID3D11Device> device) = 0;
	virtual void UpdateBuffer(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
	virtual void BindVS(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
	virtual void BindHS(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
	virtual void BindDS(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
	virtual void BindGS(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
	virtual void BindCS(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
	virtual void BindPS(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
};

template<UINT startSlot, class T>
struct CBufferObject : CBufferBase
{
	T data;

	void CreateBuffer(ComPtr<ID3D11Device> device) override
	{
		if (cBuffer != nullptr)
			return;
		D3D11_BUFFER_DESC cbd;
		ZeroMemory(&cbd, sizeof(cbd));
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.ByteWidth = sizeof(T);
		HR(device->CreateBuffer(&cbd, nullptr, cBuffer.GetAddressOf()));
	}

	void UpdateBuffer(ComPtr<ID3D11DeviceContext> deviceContext) override
	{
		if (isDirty)
		{
			isDirty = false;
			deviceContext->UpdateSubresource(cBuffer.Get(), 0, nullptr, &data, 0, 0);
		}
	}

	void BindVS(ComPtr<ID3D11DeviceContext> deviceContext) override
	{
		deviceContext->VSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindHS(ComPtr<ID3D11DeviceContext> deviceContext) override
	{
		deviceContext->HSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindDS(ComPtr<ID3D11DeviceContext> deviceContext) override
	{
		deviceContext->DSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindGS(ComPtr<ID3D11DeviceContext> deviceContext) override
	{
		deviceContext->GSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindCS(ComPtr<ID3D11DeviceContext> deviceContext) override
	{
		deviceContext->CSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindPS(ComPtr<ID3D11DeviceContext> deviceContext) override
	{
		deviceContext->PSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}
};



#endif