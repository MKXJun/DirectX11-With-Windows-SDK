
#pragma once

#include <vector>
#include <string>
#include <map>
#include <DirectXMath.h>

namespace Geometry
{
    struct MeshData
    {
        std::vector<DirectX::XMFLOAT3> vertices;
        std::vector<DirectX::XMFLOAT3> normals;
        std::vector<DirectX::XMFLOAT2> texcoords;
        std::vector<DirectX::XMFLOAT4> tangents;
        std::vector<uint32_t> indices32;
        std::vector<uint16_t> indices16;
    };


    // 创建球体网格数据，levels和slices越大，精度越高。
    MeshData CreateSphere(float radius = 1.0f, uint32_t levels = 20, uint32_t slices = 20);

    // 创建立方体网格数据
    MeshData CreateBox(float width = 2.0f, float height = 2.0f, float depth = 2.0f);

    // 创建圆柱体网格数据，slices越大，精度越高。
    MeshData CreateCylinder(float radius = 1.0f, float height = 2.0f, uint32_t slices = 20, uint32_t stacks = 10, float texU = 1.0f, float texV = 1.0f);

    // 创建圆锥体网格数据，slices越大，精度越高。
    MeshData CreateCone(float radius = 1.0f, float height = 2.0f, uint32_t slices = 20);

    
    // 创建一个平面
    MeshData CreatePlane(const DirectX::XMFLOAT2& planeSize, const DirectX::XMFLOAT2& maxTexCoord = { 1.0f, 1.0f });
    MeshData CreatePlane(float width = 10.0f, float depth = 10.0f, float texU = 1.0f, float texV = 1.0f);

}






