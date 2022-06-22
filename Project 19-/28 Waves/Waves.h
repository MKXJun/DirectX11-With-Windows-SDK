//***************************************************************************************
// Waves.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 水波加载和绘制类
// waves loader and render class.
//***************************************************************************************

#ifndef WAVERENDER_H
#define WAVERENDER_H

#include <vector>
#include <string>
#include <string_view>
#include <Vertex.h>
#include <Transform.h>
#include <Texture2D.h>
#include <GameObject.h>
#include <ModelManager.h>
#include <EffectHelper.h>
#include "Effects.h"

class Waves : public GameObject
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    uint32_t RowCount() const;
    uint32_t ColumnCount() const;

    Model* GetModel() { return &m_Model; }

protected:
    // 不允许直接构造Waves，请从CpuWaves或GpuWaves构造
    Waves() = default;
    ~Waves() = default;
    // 不允许拷贝，允许移动
    Waves(const Waves&) = delete;
    Waves& operator=(const Waves&) = delete;
    Waves(Waves&&) = default;
    Waves& operator=(Waves&&) = default;

    void InitResource(
        ID3D11Device* device,
        uint32_t rows,                  // 顶点行数
        uint32_t cols,                  // 顶点列数
        float texU,                     // 纹理坐标U方向最大值
        float texV,                     // 纹理坐标V方向最大值
        float timeStep,                 // 时间步长
        float spatialStep,              // 空间步长
        float waveSpeed,                // 波速
        float damping,                  // 粘性阻尼力
        float flowSpeedX,               // 水流X方向速度
        float flowSpeedY,               // 水流Y方向速度
        bool cpuWrite);                 // 资源CPU写

private:
    void SetModel(const Model* pModel) {}

protected:
    uint32_t m_NumRows = 0;                // 顶点行数
    uint32_t m_NumCols = 0;                // 顶点列数
    DirectX::XMFLOAT2 m_TexOffset{};

    float m_TexU = 0.0f;
    float m_TexV = 0.0f;

    float m_FlowSpeedX = 0.0f;          // 水流X方向速度
    float m_FlowSpeedY = 0.0f;          // 水流Y方向速度
    float m_TimeStep = 0.0f;            // 时间步长
    float m_SpatialStep = 0.0f;         // 空间步长
    float m_AccumulateTime = 0.0f;      // 累积时间

    Model m_Model;
    GeometryData m_MeshData;
    //
    // 我们可以预先计算出来的常量
    //

    float m_K1 = 0.0f;
    float m_K2 = 0.0f;
    float m_K3 = 0.0f;
};

class CpuWaves : public Waves
{
public:
    CpuWaves() = default;
    ~CpuWaves() = default;
    // 不允许拷贝，允许移动
    CpuWaves(const CpuWaves&) = delete;
    CpuWaves& operator=(const CpuWaves&) = delete;
    CpuWaves(CpuWaves&&) = default;
    CpuWaves& operator=(CpuWaves&&) = default;

    void InitResource(
        ID3D11Device* device,
        uint32_t rows,                  // 顶点行数
        uint32_t cols,                  // 顶点列数
        float texU,                     // 纹理坐标U方向最大值
        float texV,                     // 纹理坐标V方向最大值
        float timeStep,                 // 时间步长
        float spatialStep,              // 空间步长
        float waveSpeed,                // 波速
        float damping,                  // 粘性阻尼力
        float flowSpeedX,               // 水流X方向速度
        float flowSpeedY);              // 水流Y方向速度

    void Update(float dt);

    // 在顶点[i][j]处激起高度为magnitude的波浪
    // 仅允许在1 < i < rows和1 < j < cols的范围内激起
    void Disturb(uint32_t i, uint32_t j, float magnitude);
    // 绘制水面
    void Draw(ID3D11DeviceContext* deviceContext, IEffect& effect);

private:

    std::vector<DirectX::XMFLOAT3> m_CurrSolution;      // 保存当前模拟结果的顶点二维数组的一维展开
    std::vector<DirectX::XMFLOAT3> m_CurrNormals;       // 保存当前模拟结果的顶点法线二维数组的一维展开
    std::vector<DirectX::XMFLOAT3> m_PrevSolution;      // 保存上一次模拟结果的顶点位置二维数组的一维展开

    bool m_isUpdated = false;                           // 当前是否有顶点数据更新
};

class GpuWaves : public Waves
{
public:
    GpuWaves() = default;
    ~GpuWaves() = default;
    // 不允许拷贝，允许移动
    GpuWaves(const GpuWaves&) = delete;
    GpuWaves& operator=(const GpuWaves&) = delete;
    GpuWaves(GpuWaves&&) = default;
    GpuWaves& operator=(GpuWaves&&) = default;

    // 要求顶点行数和列数都能被16整除，以保证不会有多余
    // 的顶点被划入到新的线程组当中
    void InitResource(
        ID3D11Device* device,
        uint32_t rows,          // 顶点行数
        uint32_t cols,          // 顶点列数
        float texU,             // 纹理坐标U方向最大值
        float texV,             // 纹理坐标V方向最大值
        float timeStep,         // 时间步长
        float spatialStep,      // 空间步长
        float waveSpeed,        // 波速
        float damping,          // 粘性阻尼力
        float flowSpeedX,       // 水流X方向速度
        float flowSpeedY);      // 水流Y方向速度

    void Update(ID3D11DeviceContext* deviceContext, float dt);

    // 在顶点[i][j]处激起高度为magnitude的波浪
    // 仅允许在1 < i < rows和1 < j < cols的范围内激起
    void Disturb(ID3D11DeviceContext* deviceContext, uint32_t i, uint32_t j, float magnitude);
    // 绘制水面
    void Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect);


private:

    static std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::unique_ptr<Texture2D> m_pNextSolutionTexture;      // 缓存下一次模拟结果的y值二维数组
    std::unique_ptr<Texture2D> m_pCurrSolutionTexture;      // 保存当前模拟结果的y值二维数组
    std::unique_ptr<Texture2D> m_pPrevSolutionTexture;      // 保存上一次模拟结果的y值二维数组
};

#endif
