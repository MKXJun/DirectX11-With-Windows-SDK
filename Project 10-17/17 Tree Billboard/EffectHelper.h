//***************************************************************************************
// EffectsHelper.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 定义一些实用的特效类
// Define utility effect classes.
//***************************************************************************************

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

    CBufferBase() : isDirty() {}
    ~CBufferBase() = default;

    BOOL isDirty;
    ComPtr<ID3D11Buffer> cBuffer;

    virtual HRESULT CreateBuffer(ID3D11Device * device) = 0;
    virtual void UpdateBuffer(ID3D11DeviceContext * deviceContext) = 0;
    virtual void BindVS(ID3D11DeviceContext * deviceContext) = 0;
    virtual void BindHS(ID3D11DeviceContext * deviceContext) = 0;
    virtual void BindDS(ID3D11DeviceContext * deviceContext) = 0;
    virtual void BindGS(ID3D11DeviceContext * deviceContext) = 0;
    virtual void BindCS(ID3D11DeviceContext * deviceContext) = 0;
    virtual void BindPS(ID3D11DeviceContext * deviceContext) = 0;
};

template<UINT startSlot, class T>
struct CBufferObject : CBufferBase
{
    T data;

    CBufferObject() : CBufferBase(), data() {}

    HRESULT CreateBuffer(ID3D11Device * device) override
    {
        if (cBuffer != nullptr)
            return S_OK;
        D3D11_BUFFER_DESC cbd;
        ZeroMemory(&cbd, sizeof(cbd));
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbd.ByteWidth = sizeof(T);
        return device->CreateBuffer(&cbd, nullptr, cBuffer.GetAddressOf());
    }

    void UpdateBuffer(ID3D11DeviceContext * deviceContext) override
    {
        if (isDirty)
        {
            isDirty = false;
            D3D11_MAPPED_SUBRESOURCE mappedData;
            deviceContext->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
            memcpy_s(mappedData.pData, sizeof(T), &data, sizeof(T));
            deviceContext->Unmap(cBuffer.Get(), 0);
        }
    }

    void BindVS(ID3D11DeviceContext * deviceContext) override
    {
        deviceContext->VSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
    }

    void BindHS(ID3D11DeviceContext * deviceContext) override
    {
        deviceContext->HSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
    }

    void BindDS(ID3D11DeviceContext * deviceContext) override
    {
        deviceContext->DSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
    }

    void BindGS(ID3D11DeviceContext * deviceContext) override
    {
        deviceContext->GSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
    }

    void BindCS(ID3D11DeviceContext * deviceContext) override
    {
        deviceContext->CSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
    }

    void BindPS(ID3D11DeviceContext * deviceContext) override
    {
        deviceContext->PSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
    }
};



#endif

