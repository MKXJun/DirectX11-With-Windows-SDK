#ifndef SHADER_DEFINES_H
#define SHADER_DEFINES_H

#define MAX_LIGHTS_POWER 11
#define MAX_LIGHTS (1 << MAX_LIGHTS_POWER)
#define MAX_LIGHT_INDICES ((MAX_LIGHTS >> 3) - 1)

// 确定分块(tile)的大小用于光照去除和相关的权衡取舍 
#define COMPUTE_SHADER_TILE_GROUP_DIM 16
#define COMPUTE_SHADER_TILE_GROUP_SIZE (COMPUTE_SHADER_TILE_GROUP_DIM*COMPUTE_SHADER_TILE_GROUP_DIM)

// 如果开启，推迟逐样本着色像素的安排，直到整个tile中像素的样本0都已经着色，能够允许更好的SIMD打包和安排。
// 实际上应当总是开启该选项，因为它能够运行地更快
// 但我们现在仍保持过去的做法用于性能比较
#define DEFER_PER_SAMPLE 1

#endif // SHADER_DEFINES_H
