#include "Fire.hlsli"

[maxvertexcount(4)]
void GS(point VertexOut gIn[1], inout TriangleStream<GeoOut> output)
{
    // 不要绘制用于产生粒子的顶点
    if (gIn[0].Type != PT_EMITTER)
    {
        //
        // 计算该粒子的世界矩阵让公告板朝向摄像机
        //
        float3 look  = normalize(g_EyePosW.xyz - gIn[0].PosW);
        float3 right = normalize(cross(float3(0.0f, 1.0f, 0.0f), look));
        float3 up = cross(look, right);
        
        //
        // 计算出处于世界空间的四边形
        //
        float halfWidth  = 0.5f * gIn[0].SizeW.x;
        float halfHeight = 0.5f * gIn[0].SizeW.y;
        
        float4 v[4];
        v[0] = float4(gIn[0].PosW + halfWidth * right - halfHeight * up, 1.0f);
        v[1] = float4(gIn[0].PosW + halfWidth * right + halfHeight * up, 1.0f);
        v[2] = float4(gIn[0].PosW - halfWidth * right - halfHeight * up, 1.0f);
        v[3] = float4(gIn[0].PosW - halfWidth * right + halfHeight * up, 1.0f);
    
        //
        // 将四边形顶点从世界空间变换到齐次裁减空间
        //
        GeoOut gOut;
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            gOut.PosH  = mul(v[i], g_ViewProj);
            gOut.Tex   = g_QuadTex[i];
            gOut.Color = gIn[0].Color;
            output.Append(gOut);
        }
    }
}
