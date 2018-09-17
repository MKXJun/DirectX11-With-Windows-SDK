#ifndef BASICFX_H
#define BASICFX_H

#include <wrl/client.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include "LightHelper.h"
#include "RenderStates.h"
#include "Vertex.h"

// 由于常量缓冲区的创建需要是16字节的倍数，该函数可以返回合适的字节大小
inline UINT Align16Bytes(UINT size)
{
	return (size + 15) & (UINT)(-16);
}

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
	DirectionalLight dirLight;
	Material material;
	DirectX::XMMATRIX view;
	DirectX::XMFLOAT3 eyePos;
	float cylinderHeight;
};

class BasicFX
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// 初始化Basix.fx所需资源并初始化渲染状态
	bool InitAll(ComPtr<ID3D11Device> device);
	// 是否已经初始化
	bool IsInit() const;

	template <class T>
	void UpdateConstantBuffer(const T& cbuffer);

	// 绘制三角形分裂
	void SetRenderSplitedTriangle();
	// 绘制无上下盖的圆柱体
	void SetRenderCylinderNoCap();
	// 绘制所有顶点的法向量
	void SetRenderNormal();


private:
	// objFileNameInOut为编译好的着色器二进制文件(.*so)，若有指定则优先寻找该文件并读取
	// hlslFileName为着色器代码，若未找到着色器二进制文件则编译着色器代码
	// 编译成功后，若指定了objFileNameInOut，则保存编译好的着色器二进制信息到该文件
	// ppBlobOut输出着色器二进制信息
	HRESULT CreateShaderFromFile(const WCHAR* objFileNameInOut, const WCHAR* hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppBlobOut);

private:
	ComPtr<ID3D11VertexShader> mTriangleVS;
	ComPtr<ID3D11PixelShader> mTrianglePS;
	ComPtr<ID3D11GeometryShader> mTriangleGS;

	ComPtr<ID3D11VertexShader> mCylinderVS;
	ComPtr<ID3D11PixelShader> mCylinderPS;
	ComPtr<ID3D11GeometryShader> mCylinderGS;

	ComPtr<ID3D11VertexShader> mNormalVS;
	ComPtr<ID3D11PixelShader> mNormalPS;
	ComPtr<ID3D11GeometryShader> mNormalGS;

	ComPtr<ID3D11InputLayout> mVertexPosColorLayout;		// VertexPosColor输入布局
	ComPtr<ID3D11InputLayout> mVertexPosNormalColorLayout;	// VertexPosNormalColor输入布局

	ComPtr<ID3D11DeviceContext> md3dImmediateContext;		// 设备上下文

	std::vector<ComPtr<ID3D11Buffer>> mConstantBuffers;		// 常量缓冲区
};











#endif