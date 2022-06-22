#include "Effects.h"
#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
#include <TextureManager.h>

using namespace DirectX;

# pragma warning(disable: 26812)


//
// FXAAEffect::Impl 需要先于FXAAEffect的定义
//

class FXAAEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<EffectHelper> m_pEffectHelper;

    int m_Major = 2;
    int m_Minor = 9;
    int m_EnableDebug = 0;
    float m_QualitySubPix = 0.75f;
    float m_QualityEdgeThreshold = 0.166f;
    float m_QualityEdgeThresholdMin = 0.0833f;
};

//
// FXAAEffect
//

namespace
{
    // FXAAEffect单例
    static FXAAEffect * g_pInstance = nullptr;
}

FXAAEffect::FXAAEffect()
{
    if (g_pInstance)
        throw std::exception("FXAAEffect is a singleton!");
    g_pInstance = this;
    pImpl = std::make_unique<FXAAEffect::Impl>();
}

FXAAEffect::~FXAAEffect()
{
}

FXAAEffect::FXAAEffect(FXAAEffect && moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
}

FXAAEffect & FXAAEffect::operator=(FXAAEffect && moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
    return *this;
}

FXAAEffect & FXAAEffect::Get()
{
    if (!g_pInstance)
        throw std::exception("FXAAEffect needs an instance!");
    return *g_pInstance;
}


bool FXAAEffect::InitAll(ID3D11Device * device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"Shaders\\Cache");
    
    const char* numStrs[] = { 
        "10", "11", "12", "13", "14", "15", 
        "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
        "39"};
    D3D_SHADER_MACRO defines[] =
    {
        { "FXAA_QUALITY__PRESET", "39" },
        { nullptr, nullptr },
        { nullptr, nullptr }
    };

    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    // 创建顶点着色器
    pImpl->m_pEffectHelper->CreateShaderFromFile("FullScreenTriangleTexcoordVS", L"Shaders/FXAA.hlsl", device,
        "FullScreenTriangleTexcoordVS", "vs_5_0");
    

    // 前三位代表
    // [质量主级别][质量次级别][调试模式]
    std::string psName = "000_PS";
    std::string passName = "000_FXAA";
    EffectPassDesc passDesc;
    
    // 创建通道
    passDesc.nameVS = "FullScreenTriangleTexcoordVS";
    

    for (auto str : numStrs)
    {
        psName[0] = passName[0] = str[0];
        psName[1] = passName[1] = str[1];
        psName[2] = passName[2] = '1';
        defines[1].Name = "DEBUG_OUTPUT";
        defines[1].Definition = "";
        // 创建像素着色器
        pImpl->m_pEffectHelper->CreateShaderFromFile(psName, L"Shaders/FXAA.hlsl", device,
            "PS", "ps_5_0", defines);

        // 创建通道
        passDesc.namePS = psName;
        HR(pImpl->m_pEffectHelper->AddEffectPass(passName, device, &passDesc));

        psName[2] = passName[2] = '0';
        defines[1].Name = nullptr;
        defines[1].Definition = nullptr;

        // 创建像素着色器
        pImpl->m_pEffectHelper->CreateShaderFromFile(psName, L"Shaders/FXAA.hlsl", device,
            "PS", "ps_5_0", defines);
        // 创建通道
        passDesc.namePS = psName;
        HR(pImpl->m_pEffectHelper->AddEffectPass(passName, device, &passDesc));
    }
    
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamplerLinearClamp", RenderStates::SSLinearClamp.Get());

    // 设置调试对象名
    pImpl->m_pEffectHelper->SetDebugObjectName("FXAAEffect");

    return true;
}

void FXAAEffect::SetQuality(int major, int minor)
{
    pImpl->m_Major = std::clamp(major, 1, 3);
    switch (major)
    {
    case 1: pImpl->m_Minor = std::clamp(minor, 0, 5); break;
    case 2: pImpl->m_Minor = std::clamp(minor, 0, 9); break;
    case 3: pImpl->m_Minor = 9; break;
    default:
        break;
    }
}

void FXAAEffect::SetQualitySubPix(float val)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_QualitySubPix")->SetFloat(val);
}

void FXAAEffect::SetQualityEdgeThreshold(float threshold)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_QualityEdgeThreshold")->SetFloat(threshold);
}

void FXAAEffect::SetQualityEdgeThresholdMin(float thresholdMin)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_QualityEdgeThresholdMin")->SetFloat(thresholdMin);
}

void FXAAEffect::EnableDebug(bool enabled)
{
    pImpl->m_EnableDebug = !!enabled;
}

void FXAAEffect::RenderFXAA(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input, 
    ID3D11RenderTargetView* output, 
    const D3D11_VIEWPORT& vp)
{
    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    std::string passName = "000_FXAA";
    passName[0] = '0' + pImpl->m_Major;
    passName[1] = '0' + pImpl->m_Minor;
    passName[2] = '0' + pImpl->m_EnableDebug;
    auto pass = pImpl->m_pEffectHelper->GetEffectPass(passName);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureInput", input);
    float texelSizes[2] = { 1.0f / vp.Width, 1.0f / vp.Height };
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TexelSize")->SetFloatVector(2, texelSizes);
    pass->Apply(deviceContext);
    deviceContext->OMSetRenderTargets(1, &output, nullptr);
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->Draw(3, 0);

    int slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_TextureInput");
    input = nullptr;
    deviceContext->PSSetShaderResources(slot, 1, &input);
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}



