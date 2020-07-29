#include "Shadow.hlsli"

// 这仅仅用于Alpha几何裁剪，以保证阴影的显示正确。
// 对于不需要进行纹理采样操作的几何体可以直接将像素
// 着色器设为nullptr
void PS(VertexPosHTex pIn)
{
    float4 diffuse = g_DiffuseMap.Sample(g_Sam, pIn.Tex);
    
    // 不要将透明像素写入深度贴图
    clip(diffuse.a - 0.1f);
}
