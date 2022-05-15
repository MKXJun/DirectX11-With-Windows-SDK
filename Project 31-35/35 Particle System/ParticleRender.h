//***************************************************************************************
// ParticleRender.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 粒子渲染类
// Particle Render class.
//***************************************************************************************

#ifndef _PARTICLERENDER_H_
#define _PARTICLERENDER_H_

#include "Effects.h"
#include "Camera.h"

class ParticleRender
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ParticleRender() = default;
    ~ParticleRender() = default;
    // 不允许拷贝，允许移动
    ParticleRender(const ParticleRender&) = delete;
    ParticleRender& operator=(const ParticleRender&) = delete;
    ParticleRender(ParticleRender&&) = default;
    ParticleRender& operator=(ParticleRender&&) = default;

    // 自从该系统被重置以来所经过的时间
    float GetAge() const;

    void SetEmitPos(const DirectX::XMFLOAT3& emitPos);
    void SetEmitDir(const DirectX::XMFLOAT3& emitDir);

    void SetEmitInterval(float t);
    void SetAliveTime(float t);

    HRESULT Init(ID3D11Device* device, UINT maxParticles);
    void SetTextureArraySRV(ID3D11ShaderResourceView* textureArraySRV);
    void SetRandomTexSRV(ID3D11ShaderResourceView* randomTexSRV);

    void Reset();
    void Update(float dt, float gameTime);
    void Draw(ID3D11DeviceContext* deviceContext, ParticleEffect& effect, const Camera& camera);

    void SetDebugObjectName(const std::string& name);

private:
    
    UINT m_MaxParticles = 0;
    bool m_FirstRun = true;

    float m_GameTime = 0.0f;
    float m_TimeStep = 0.0f;
    float m_Age = 0.0f;

    DirectX::XMFLOAT3 m_EmitPos = {};
    DirectX::XMFLOAT3 m_EmitDir = {};

    float m_EmitInterval = 0.0f;
    float m_AliveTime = 0.0f;

    ComPtr<ID3D11Buffer> m_pInitVB;
    ComPtr<ID3D11Buffer> m_pDrawVB;
    ComPtr<ID3D11Buffer> m_pStreamOutVB;

    ComPtr<ID3D11ShaderResourceView> m_pTextureArraySRV;
    ComPtr<ID3D11ShaderResourceView> m_pRandomTexSRV;
    
};

#endif
