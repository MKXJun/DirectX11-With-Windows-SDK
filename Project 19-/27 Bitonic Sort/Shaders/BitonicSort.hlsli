Buffer<uint> g_Input : register(t0);
RWBuffer<uint> g_Data : register(u0);

cbuffer CB : register(b0)
{
    uint g_Level;        // 2^需要排序趟数
    uint g_DescendMask;  // 下降序列掩码
    uint g_MatrixWidth;  // 矩阵宽度(要求宽度>=高度且都为2的倍数)
    uint g_MatrixHeight; // 矩阵高度
}