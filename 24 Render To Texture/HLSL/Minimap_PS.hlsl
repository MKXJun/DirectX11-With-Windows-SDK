#include "Minimap.hlsli"

// 像素着色器
float4 PS(VertexPosHTex pIn) : SV_Target
{
    // 要求Tex的取值范围都在[0.0f, 1.0f], y值对应世界坐标z轴
    float2 PosW = pIn.Tex * float2(gRectW.zw - gRectW.xy) + gRectW.xy;
    
    float4 color = gTex.Sample(gSam, pIn.Tex);

    [flatten]
    if (gFogEnabled && length(PosW - gEyePosW.xz) / gVisibleRange > 1.0f)
    {
        return gInvisibleColor;
    }

    return color;
}