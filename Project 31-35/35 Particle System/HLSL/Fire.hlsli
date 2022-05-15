
cbuffer CBChangesEveryFrame : register(b0)
{
    matrix g_ViewProj;
    
    float3 g_EyePosW;
    float g_GameTime;
    
    float g_TimeStep;
    float3 g_EmitDirW;
    
    float3 g_EmitPosW;
    float g_EmitInterval;
    
    float g_AliveTime;
}

cbuffer CBFixed : register(b1)
{
    // 用于加速粒子运动的加速度
    float3 g_AccelW = float3(0.0f, 7.8f, 0.0f);
    
    // 纹理坐标
    float2 g_QuadTex[4] =
    {
        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f)
    };
}

// 用于贴图到粒子上的纹理数组
Texture2DArray g_TexArray : register(t0);

// 用于在着色器中生成随机数的纹理
Texture1D g_RandomTex : register(t1);

// 采样器
SamplerState g_SamLinear : register(s0);


float3 RandUnitVec3(float offset)
{
    // 使用游戏时间加上偏移值来采样随机纹理
    float u = (g_GameTime + offset);
    
    // 采样值在[-1,1]
    float3 v = g_RandomTex.SampleLevel(g_SamLinear, u, 0).xyz;
    
    // 投影到单位球
    return normalize(v);
}

#define PT_EMITTER 0
#define PT_FLARE 1

struct VertexParticle
{
    float3 InitialPosW : POSITION;
    float3 InitialVelW : VELOCITY;
    float2 SizeW : SIZE;
    float Age : AGE;
    uint Type : TYPE;
};

// 绘制输出
struct VertexOut
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
    float4 Color : COLOR;
    uint Type : TYPE;
};

struct GeoOut
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD;
};

