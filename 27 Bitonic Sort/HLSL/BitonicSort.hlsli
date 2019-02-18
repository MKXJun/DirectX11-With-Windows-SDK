Buffer<uint> gInput : register(t0);
RWBuffer<uint> gData : register(u0);

cbuffer CB : register(b0)
{
    uint gLevel;        // 2^需要排序趟数
    uint gDescendMask;  // 下降序列掩码
    uint gMatrixWidth;  // 矩阵宽度(要求宽度>=高度且都为2的倍数)
    uint gMatrixHeight; // 矩阵高度
}