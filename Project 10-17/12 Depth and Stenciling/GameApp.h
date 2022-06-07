#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <d3dApp.h>
#include <Geometry.h>
#include <LightHelper.h>
#include <Camera.h>
#include <RenderStates.h>

class GameApp : public D3DApp
{
public:

    struct CBChangesEveryDrawing
    {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX worldInvTranspose;
        Material material;
    };

    struct CBDrawingStates
    {
        int isReflection;
        DirectX::XMFLOAT3 pad;
    };

    struct CBChangesEveryFrame
    {
        DirectX::XMMATRIX view;
        DirectX::XMFLOAT4 eyePos;

    };

    struct CBChangesOnResize
    {
        DirectX::XMMATRIX proj;
    };


    struct CBChangesRarely
    {
        DirectX::XMMATRIX reflection;
        DirectionalLight dirLight[10];
        PointLight pointLight[10];
        SpotLight spotLight[10];
        int numDirLight;
        int numPointLight;
        int numSpotLight;
        float pad;		// 打包保证16字节对齐
    };

    // 一个尽可能小的游戏对象类
    class GameObject
    {
    public:
        GameObject();

        // 获取物体变换
        Transform& GetTransform();
        // 获取物体变换
        const Transform& GetTransform() const;

        // 设置缓冲区
        template<class VertexType, class IndexType>
        void SetBuffer(ID3D11Device* device, const Geometry::MeshData<VertexType, IndexType>& meshData);
        // 设置纹理
        void SetTexture(ID3D11ShaderResourceView* texture);
        // 设置材质
        void SetMaterial(const Material& material);

        // 绘制
        void Draw(ID3D11DeviceContext* deviceContext);

        // 设置调试对象名
        // 若缓冲区被重新设置，调试对象名也需要被重新设置
        void SetDebugObjectName(const std::string& name);
    private:
        Transform m_Transform;								// 物体变换信息
        Material m_Material;								// 物体材质
        ComPtr<ID3D11ShaderResourceView> m_pTexture;		// 纹理
        ComPtr<ID3D11Buffer> m_pVertexBuffer;				// 顶点缓冲区
        ComPtr<ID3D11Buffer> m_pIndexBuffer;				// 索引缓冲区
        UINT m_VertexStride;								// 顶点字节大小
        UINT m_IndexCount;								    // 索引数目	
    };

    // 摄像机模式
    enum class CameraMode { FirstPerson, ThirdPerson, Free };
    
public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    bool InitEffect();
    bool InitResource();

private:   

    ComPtr<ID3D11InputLayout> m_pVertexLayout2D;				// 用于2D的顶点输入布局
    ComPtr<ID3D11InputLayout> m_pVertexLayout3D;				// 用于3D的顶点输入布局
    ComPtr<ID3D11Buffer> m_pConstantBuffers[5];				    // 常量缓冲区

    GameObject m_WireFence;									    // 篱笆盒
    GameObject m_Floor;										    // 地板
    std::vector<GameObject> m_Walls;							// 墙壁
    GameObject m_Water;										    // 水
    GameObject m_Mirror;										// 镜面

    ComPtr<ID3D11VertexShader> m_pVertexShader3D;				// 用于3D的顶点着色器
    ComPtr<ID3D11PixelShader> m_pPixelShader3D;				    // 用于3D的像素着色器
    ComPtr<ID3D11VertexShader> m_pVertexShader2D;				// 用于2D的顶点着色器
    ComPtr<ID3D11PixelShader> m_pPixelShader2D;				    // 用于2D的像素着色器

    CBDrawingStates m_CBStates;								    // 该缓冲区存放绘制状态的变量 
    CBChangesEveryFrame m_CBFrame;							    // 该缓冲区存放仅在每一帧进行更新的变量
    CBChangesOnResize m_CBOnResize;							    // 该缓冲区存放仅在窗口大小变化时更新的变量
    CBChangesRarely m_CBRarely;								    // 该缓冲区存放不会再进行修改的变量

    std::shared_ptr<Camera> m_pCamera;						    // 摄像机
    CameraMode m_CameraMode;									// 摄像机模式

};


#endif