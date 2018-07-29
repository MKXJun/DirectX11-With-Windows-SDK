#ifndef BASICMANAGER_H
#define BASICMANAGER_H

#include <wrl/client.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include "LightHelper.h"
#include "RenderStates.h"
#include "Vertex.h"

// 由于常量缓冲区的创建需要是16字节的倍数，该函数可以返回合适的字节大小
inline size_t Align16Bytes(size_t size)
{
	return (size + 15) & (size_t)(-16);
}

struct CBChangesEveryDrawing
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX texTransform;
	Material material;
};

struct CBChangesEveryFrame
{
	DirectX::XMMATRIX view;
	DirectX::XMFLOAT4 eyePos;

};

struct CBDrawingState
{
	int isReflection;
	int isShadow;
	DirectX::XMINT2 pad;
};

struct CBChangesOnResize
{
	DirectX::XMMATRIX proj;
};


struct CBNeverChange
{
	DirectX::XMMATRIX reflection;
	DirectX::XMMATRIX shadow;
	DirectX::XMMATRIX refShadow;
	DirectionalLight dirLight[10];
	PointLight pointLight[10];
	SpotLight spotLight[10];
	int numDirLight;
	int numPointLight;
	int numSpotLight;
	float pad;		// 打包保证16字节对齐
};

class BasicManager
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// 初始化Basix.fx所需资源并初始化光栅化状态
	bool InitAll(ComPtr<ID3D11Device> device);
	// 是否已经初始化
	bool IsInit() const;

	template <class T>
	void UpdateConstantBuffer(const T& cbuffer);

	// 默认状态来绘制
	void SetRenderDefault();
	// Alpha混合绘制
	void SetRenderAlphaBlend();		
	// 无二次混合
	void SetRenderNoDoubleBlend(UINT stencilRef);
	// 仅写入模板值
	void SetWriteStencilOnly(UINT stencilRef);		
	// 对指定模板值的区域进行绘制，采用默认状态
	void SetRenderDefaultWithStencil(UINT stencilRef);		
	// 对指定模板值的区域进行绘制，采用Alpha混合
	void SetRenderAlphaBlendWithStencil(UINT stencilRef);		
	// 2D默认状态绘制
	void Set2DRenderDefault();
	// 2D混合绘制
	void Set2DRenderAlphaBlend();


private:
	// 从.fx/.hlsl文件中编译着色器
	HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

private:
	ComPtr<ID3D11VertexShader> mVertexShader3D;				// 用于3D的顶点着色器
	ComPtr<ID3D11PixelShader> mPixelShader3D;				// 用于3D的像素着色器
	ComPtr<ID3D11VertexShader> mVertexShader2D;				// 用于2D的顶点着色器
	ComPtr<ID3D11PixelShader> mPixelShader2D;				// 用于2D的像素着色器

	ComPtr<ID3D11InputLayout> mVertexLayout2D;				// 用于2D的顶点输入布局
	ComPtr<ID3D11InputLayout> mVertexLayout3D;				// 用于3D的顶点输入布局

	ComPtr<ID3D11DeviceContext> md3dImmediateContext;		// 设备上下文

	std::vector<ComPtr<ID3D11Buffer>> mConstantBuffers;		// 常量缓冲区
};











#endif