#ifndef EFFECTS_H
#define EFFECTS_H
#include <vector>
#include <memory>
#include "LightHelper.h"
#include "RenderStates.h"


class IEffect
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	IEffect() = default;

	// 不支持复制构造
	IEffect(const IEffect&) = delete;
	IEffect& operator=(const IEffect&) = delete;

	// 允许转移
	IEffect(IEffect&& moveFrom) = default;
	IEffect& operator=(IEffect&& moveFrom) = default;

	virtual ~IEffect() = default;

	// 更新并绑定常量缓冲区
	virtual void Apply(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
};


class BasicObjectFX : public IEffect
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	BasicObjectFX();
	virtual ~BasicObjectFX() override;

	BasicObjectFX(BasicObjectFX&& moveFrom);
	BasicObjectFX& operator=(BasicObjectFX&& moveFrom);

	// 获取单例
	static BasicObjectFX& Get();

	

	// 初始化Basix.fx所需资源并初始化渲染状态
	bool InitAll(ComPtr<ID3D11Device> device);


	//
	// 渲染模式的变更
	//

	// 绘制三角形分形
	void SetRenderSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext);
	// 绘制雪花
	void SetRenderSplitedSnow(ComPtr<ID3D11DeviceContext> deviceContext);
	// 绘制球体
	void SetRenderSplitedSphere(ComPtr<ID3D11DeviceContext> deviceContext);
	// 通过流输出阶段获取三角形分裂的下一阶分形
	void SetStreamOutputSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut);
	// 通过流输出阶段获取雪花的下一阶分形
	void SetStreamOutputSplitedSnow(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut);
	// 通过流输出阶段获取球的下一阶分形
	void SetStreamOutputSplitedSphere(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut);

	// 绘制所有顶点的法向量
	void SetRenderNormal(ComPtr<ID3D11DeviceContext> deviceContext);


	//
	// 矩阵设置
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W);
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V);
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P);
	void XM_CALLCONV SetWorldViewProjMatrix(DirectX::FXMMATRIX W, DirectX::CXMMATRIX V, DirectX::CXMMATRIX P);

	
	//
	// 光照、材质和纹理相关设置
	//

	// 各种类型灯光允许的最大数目
	static const int maxLights = 5;

	void SetDirLight(size_t pos, const DirectionalLight& dirLight);
	void SetPointLight(size_t pos, const PointLight& pointLight);
	void SetSpotLight(size_t pos, const SpotLight& spotLight);

	void SetMaterial(const Material& material);



	void XM_CALLCONV SetEyePos(DirectX::FXMVECTOR eyePos);

	//
	// 设置球体
	//

	void SetSphereCenter(const DirectX::XMFLOAT3& center);
	void SetSphereRadius(float radius);

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ComPtr<ID3D11DeviceContext> deviceContext);
	
private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};










#endif