#include "Basic.hlsli"

// 节省内存资源，先用float4向量声明。
static const float4 g_Vec[2] = { float4(0.0f, 1.0f, 0.0f, 0.0f), float4(1.0f, 1.0f, 1.0f, 0.0f) };
static const float2 g_TexCoord[4] = (float2[4])g_Vec;

[maxvertexcount(4)]
void GS(point PointSprite input[1], uint primID : SV_PrimitiveID, 
    inout TriangleStream<BillboardVertex> output)
{
    // 计算公告板所处的局部坐标系，其中公告板相当于
    // 被投影在了局部坐标系的xy平面，z=0

    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = g_EyePosW - input[0].PosW;
    look.y = 0.0f;  // look向量只取投影到xz平面的向量
    look = normalize(look);
    float3 right = cross(up, look);

    // 计算出公告板矩形的四个顶点
    //            up
    //      v1 ___|___ v3
    //        |   |   |
    // right__|___|   |
    //        |  /    |
    //        |_/_____|
    //      v0 /       v2
    //       look  
    float4 v[4];
    float3 center = input[0].PosW;
    float halfWidth = 0.5f * input[0].SizeW.x;
    float halfHeight = 0.5f * input[0].SizeW.y;
    v[0] = float4(center + halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(center + halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(center - halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(center - halfWidth * right + halfHeight * up, 1.0f);

    // 对顶点位置进行矩阵变换，并以TriangleStrip形式输出
    BillboardVertex gOut;
    matrix viewProj = mul(g_View, g_Proj);
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gOut.PosW = v[i].xyz;
        gOut.PosH = mul(v[i], viewProj);
        gOut.NormalW = look;
        gOut.Tex = g_TexCoord[i];
        gOut.PrimID = primID;
        output.Append(gOut);
    }

}