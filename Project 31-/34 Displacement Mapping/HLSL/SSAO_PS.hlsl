#include "SSAO.hlsli"

// 给定点r和p的深度差，计算出采样点q对点p的遮蔽程度
float OcclusionFunction(float distZ)
{
    //
    // 如果depth(q)在depth(p)之后(超出半球范围)，那点q不能遮蔽点p。此外，如果depth(q)和depth(p)过于接近，
    // 我们也认为点q不能遮蔽点p，因为depth(p)-depth(r)需要超过用户假定的Epsilon值才能认为点q可以遮蔽点p
    //
    // 我们用下面的函数来确定遮蔽程度
    //
    //    /|\ Occlusion
    // 1.0 |      ---------------\
    //     |      |             |  \
    //     |                         \
    //     |      |             |      \
    //     |                             \
    //     |      |             |          \
    //     |                                 \
    // ----|------|-------------|-------------|-------> zv
    //     0     Eps          zStart         zEnd
    float occlusion = 0.0f;
    if (distZ > g_SurfaceEpsilon)
    {
        float fadeLength = g_OcclusionFadeEnd - g_OcclusionFadeStart;
        // 当distZ由g_OcclusionFadeStart逐渐趋向于g_OcclusionFadeEnd，遮蔽值由1线性减小至0
        occlusion = saturate((g_OcclusionFadeEnd - distZ) / fadeLength);
    }
    return occlusion;
}


// 绘制SSAO图的顶点着色器
float4 PS(VertexOut pIn, uniform int sampleCount) : SV_TARGET
{
    // p -- 我们要计算的环境光遮蔽目标点
    // n -- 顶点p的法向量
    // q -- 点p处所在半球内的随机一点
    // r -- 有可能遮挡点p的一点
    
    // 获取观察空间的法向量和当前像素的z坐标
    float4 normalDepth = g_NormalDepthMap.SampleLevel(g_SamNormalDepth, pIn.Tex, 0.0f);
    
    float3 n = normalDepth.xyz;
    float pz = normalDepth.w;
    
    //
    // 重建观察空间坐标 (x, y, z)
    // 寻找t使得能够满足 p = t * pIn.ToFarPlane
    // p.z = t * pIn.ToFarPlane.z
    // t = p.z / pIn.ToFarPlane.z
    //
    float3 p = (pz / pIn.ToFarPlane.z) * pIn.ToFarPlane;
    
    // 获取随机向量并从[0, 1]^3映射到[-1, 1]^3
    float3 randVec = g_RandomVecMap.SampleLevel(g_SamRandomVec, 4.0f * pIn.Tex, 0.0f).xyz;
    randVec = 2.0f * randVec - 1.0f;
    
    float occlusionSum = 0.0f;
    
    // 在以p为中心的半球内，根据法线n对p周围的点进行采样
    for (int i = 0; i < sampleCount; ++i)
    {
        // 偏移向量都是固定且均匀分布的(所以我们采用的偏移向量不会在同一方向上扎堆)。
        // 如果我们将这些偏移向量关联于一个随机向量进行反射，则得到的必定为一组均匀分布
        // 的随机偏移向量
        float3 offset = reflect(g_OffsetVectors[i].xyz, randVec);
        
        // 如果偏移向量位于(p, n)定义的平面之后，将其翻转
        float flip = sign(dot(offset, n));
        
        // 在点p处于遮蔽半径的半球范围内进行采样
        float3 q = p + flip * g_OcclusionRadius * offset;
    
        // 将q进行投影，得到投影纹理坐标
        float4 projQ = mul(float4(q, 1.0f), g_ViewToTexSpace);
        projQ /= projQ.w;
        
        // 找到眼睛观察点q方向所能观察到的最近点r所处的深度值(有可能点r不存在，此时观察到
        // 的是远平面上一点)。为此，我们需要查看此点在深度图中的深度值
        float rz = g_NormalDepthMap.SampleLevel(g_SamNormalDepth, projQ.xy, 0.0f).w;
        
        // 重建点r在观察空间中的坐标 r = (rx, ry, rz)
        // 我们知道点r位于眼睛到点q的射线上，故有r = t * q
        // r.z = t * q.z ==> t = t.z / q.z
        float3 r = (rz / q.z) * q;
        
        // 测试点r是否遮蔽p
        //   - 点积dot(n, normalize(r - p))度量遮蔽点r到平面(p, n)前侧的距离。越接近于
        //     此平面的前侧，我们就给它设定越大的遮蔽权重。同时，这也能防止位于倾斜面
        //     (p, n)上一点r的自阴影所产生出错误的遮蔽值(通过设置g_SurfaceEpsilon)，这
        //     是因为在以观察点的视角来看，它们有着不同的深度值，但事实上，位于倾斜面
        //     (p, n)上的点r却没有遮挡目标点p
        //   - 遮蔽权重的大小取决于遮蔽点与其目标点之间的距离。如果遮蔽点r离目标点p过
        //     远，则认为点r不会遮挡点p
        
        float distZ = p.z - r.z;
        float dp = max(dot(n, normalize(r - p)), 0.0f);
        float occlusion = dp * OcclusionFunction(distZ);
        
        occlusionSum += occlusion;
    }
    
    occlusionSum /= sampleCount;
    
    float access = 1.0f - occlusionSum;
    
    // 增强SSAO图的对比度，是的SSAO图的效果更加明显
    return saturate(pow(access, 4.0f));
}
