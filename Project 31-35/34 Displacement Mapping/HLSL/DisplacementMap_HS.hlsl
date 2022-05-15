#include "Basic.hlsli"

PatchTess PatchHS(InputPatch<TessVertexOut, 3> patch,
                  uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    // 对每条边的曲面细分因子求平均值，并选择其中一条边的作为其内部的
    // 曲面细分因子。基于边的属性来进行曲面细分因子的计算非常重要，这
    // 样那些与多个三角形共享的边将会拥有相同的曲面细分因子，否则会导
    // 致间隙的产生
    pt.EdgeTess[0] = 0.5f * (patch[1].TessFactor + patch[2].TessFactor);
    pt.EdgeTess[1] = 0.5f * (patch[2].TessFactor + patch[0].TessFactor);
    pt.EdgeTess[2] = 0.5f * (patch[0].TessFactor + patch[1].TessFactor);
    pt.InsideTess = pt.EdgeTess[0];
    
    return pt;
}

// 外壳着色器
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchHS")]
HullOut HS(InputPatch<TessVertexOut, 3> patch,
    uint i : SV_OutputControlPointID,
    uint patchId : SV_PrimitiveID)
{
    HullOut hOut;
    
    // 直传
    hOut.PosW = patch[i].PosW;
    hOut.NormalW = patch[i].NormalW;
    hOut.TangentW = patch[i].TangentW;
    hOut.Tex = patch[i].Tex;
    
    return hOut;
}

