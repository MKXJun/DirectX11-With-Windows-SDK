//***************************************************************************************
// WavesRender.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 水波加载和绘制类
// waves loader and render class.
//***************************************************************************************

#ifndef WAVERENDER_H
#define WAVERENDER_H

#include <vector>
#include <string>
#include "Vertex.h"
#include "Effects.h"
#include "Transform.h"

class WavesRender
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	void SetMaterial(const Material& material);

	Transform& GetTransform();
	const Transform& GetTransform() const;

	UINT RowCount() const;
	UINT ColumnCount() const;

protected:
	// 不允许直接构造WavesRender，请从CpuWavesRender或GpuWavesRender构造
	WavesRender() = default;
	~WavesRender() = default;
	// 不允许拷贝，允许移动
	WavesRender(const WavesRender&) = delete;
	WavesRender& operator=(const WavesRender&) = delete;
	WavesRender(WavesRender&&) = default;
	WavesRender& operator=(WavesRender&&) = default;

	void Init(
		UINT rows,			// 顶点行数
		UINT cols,			// 顶点列数
		float texU,			// 纹理坐标U方向最大值
		float texV,			// 纹理坐标V方向最大值
		float timeStep,		// 时间步长
		float spatialStep,	// 空间步长
		float waveSpeed,	// 波速
		float damping,		// 粘性阻尼力
		float flowSpeedX,	// 水流X方向速度
		float flowSpeedY);	// 水流Y方向速度

protected:
	UINT m_NumRows = 0;					// 顶点行数
	UINT m_NumCols = 0;					// 顶点列数

	UINT m_VertexCount = 0;				// 顶点数目
	UINT m_IndexCount = 0;				// 索引数目

	Transform m_Transform = {};			// 水面变换
	DirectX::XMFLOAT2 m_TexOffset = {};	// 纹理坐标偏移
	float m_TexU = 0.0f;				// 纹理坐标U方向最大值
	float m_TexV = 0.0f;				// 纹理坐标V方向最大值
	Material m_Material = {};			// 水面材质

	float m_FlowSpeedX = 0.0f;			// 水流X方向速度
	float m_FlowSpeedY = 0.0f;			// 水流Y方向速度
	float m_TimeStep = 0.0f;			// 时间步长
	float m_SpatialStep = 0.0f;			// 空间步长
	float m_AccumulateTime = 0.0f;		// 累积时间

	//
	// 我们可以预先计算出来的常量
	//

	float m_K1 = 0.0f;
	float m_K2 = 0.0f;
	float m_K3 = 0.0f;

	
};

class CpuWavesRender : public WavesRender
{
public:
	CpuWavesRender() = default;
	~CpuWavesRender() = default;
	// 不允许拷贝，允许移动
	CpuWavesRender(const CpuWavesRender&) = delete;
	CpuWavesRender& operator=(const CpuWavesRender&) = delete;
	CpuWavesRender(CpuWavesRender&&) = default;
	CpuWavesRender& operator=(CpuWavesRender&&) = default;

	HRESULT InitResource(ID3D11Device* device,
		const std::wstring& texFileName,	// 纹理文件名
		UINT rows,			// 顶点行数
		UINT cols,			// 顶点列数
		float texU,			// 纹理坐标U方向最大值
		float texV,			// 纹理坐标V方向最大值
		float timeStep,		// 时间步长
		float spatialStep,	// 空间步长
		float waveSpeed,	// 波速
		float damping,		// 粘性阻尼力
		float flowSpeedX,	// 水流X方向速度
		float flowSpeedY);	// 水流Y方向速度

	void Update(float dt);
	
	// 在顶点[i][j]处激起高度为magnitude的波浪
	// 仅允许在1 < i < rows和1 < j < cols的范围内激起
	void Disturb(UINT i, UINT j, float magnitude);
	// 绘制水面
	void Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect);

	void SetDebugObjectName(const std::string& name);
private:
	

	std::vector<VertexPosNormalTex> m_Vertices;			// 保存当前模拟结果的顶点二维数组的一维展开
	std::vector<VertexPos> m_PrevSolution;				// 保存上一次模拟结果的顶点位置二维数组的一维展开

	ComPtr<ID3D11Buffer> m_pVertexBuffer;				// 当前模拟的顶点缓冲区
	ComPtr<ID3D11Buffer> m_pIndexBuffer;				// 当前模拟的索引缓冲区

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;	// 水面纹理
	bool m_isUpdated = false;							// 当前是否有顶点数据更新
};

class GpuWavesRender : public WavesRender
{
public:
	GpuWavesRender() = default;
	~GpuWavesRender() = default;
	// 不允许拷贝，允许移动
	GpuWavesRender(const GpuWavesRender&) = delete;
	GpuWavesRender& operator=(const GpuWavesRender&) = delete;
	GpuWavesRender(GpuWavesRender&&) = default;
	GpuWavesRender& operator=(GpuWavesRender&&) = default;

	// 要求顶点行数和列数都能被16整除，以保证不会有多余
	// 的顶点被划入到新的线程组当中
	HRESULT InitResource(ID3D11Device* device,
		const std::wstring& texFileName,	// 纹理文件名
		UINT rows,			// 顶点行数
		UINT cols,			// 顶点列数
		float texU,			// 纹理坐标U方向最大值
		float texV,			// 纹理坐标V方向最大值
		float timeStep,		// 时间步长
		float spatialStep,	// 空间步长
		float waveSpeed,	// 波速
		float damping,		// 粘性阻尼力
		float flowSpeedX,	// 水流X方向速度
		float flowSpeedY);	// 水流Y方向速度

	void Update(ID3D11DeviceContext* deviceContext, float dt);

	// 在顶点[i][j]处激起高度为magnitude的波浪
	// 仅允许在1 < i < rows和1 < j < cols的范围内激起
	void Disturb(ID3D11DeviceContext* deviceContext, UINT i, UINT j, float magnitude);
	// 绘制水面
	void Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect);

	void SetDebugObjectName(const std::string& name);

private:
	struct {
		DirectX::XMFLOAT4 waveInfo;
		DirectX::XMINT4 index;
	} m_CBUpdateSettings = {};								// 对应Waves.hlsli的常量缓冲区

private:
	ComPtr<ID3D11Texture2D> m_pNextSolution;				// 缓存下一次模拟结果的y值二维数组
	ComPtr<ID3D11Texture2D> m_pCurrSolution;				// 保存当前模拟结果的y值二维数组
	ComPtr<ID3D11Texture2D> m_pPrevSolution;				// 保存上一次模拟结果的y值二维数组

	ComPtr<ID3D11ShaderResourceView> m_pNextSolutionSRV;	// 缓存下一次模拟结果的y值着色器资源视图
	ComPtr<ID3D11ShaderResourceView> m_pCurrSolutionSRV;	// 缓存当前模拟结果的y值着色器资源视图
	ComPtr<ID3D11ShaderResourceView> m_pPrevSolutionSRV;	// 缓存上一次模拟结果的y值着色器资源视图

	ComPtr<ID3D11UnorderedAccessView> m_pNextSolutionUAV;	// 缓存下一次模拟结果的y值无序访问视图
	ComPtr<ID3D11UnorderedAccessView> m_pCurrSolutionUAV;	// 缓存当前模拟结果的y值无序访问视图
	ComPtr<ID3D11UnorderedAccessView> m_pPrevSolutionUAV;	// 缓存上一次模拟结果的y值无序访问视图

	ComPtr<ID3D11Buffer> m_pVertexBuffer;					// 当前模拟的顶点缓冲区
	ComPtr<ID3D11Buffer> m_pIndexBuffer;					// 当前模拟的索引缓冲区
	ComPtr<ID3D11Buffer> m_pConstantBuffer;					// 当前模拟的常量缓冲区

	ComPtr<ID3D11ComputeShader> m_pWavesUpdateCS;			// 用于计算模拟结果的着色器
	ComPtr<ID3D11ComputeShader> m_pWavesDisturbCS;			// 用于激起水波的着色器

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;		// 水面纹理
};

#endif