#include "DebugTexture.hlsli"

float4 PS(VertexPosHTex pIn, uniform int index) : SV_Target
{
    float comp[4] = (float[4])g_DiffuseMap.Sample(g_Sam, pIn.Tex);
    
    float4 output = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float outComp[4] = (float[4]) output;
    
    outComp[index] = comp[index];
    
    return float4(outComp[0], outComp[1], outComp[2], outComp[3]);
}
