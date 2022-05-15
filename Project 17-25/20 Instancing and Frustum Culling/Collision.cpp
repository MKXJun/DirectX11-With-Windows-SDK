#include "Collision.h"

using namespace DirectX;

Collision::WireFrameData Collision::CreateBoundingBox(const DirectX::BoundingBox & box, const DirectX::XMFLOAT4 & color)
{
    XMFLOAT3 corners[8];
    box.GetCorners(corners);
    return CreateFromCorners(corners, color);
}

Collision::WireFrameData Collision::CreateBoundingOrientedBox(const DirectX::BoundingOrientedBox & box, const DirectX::XMFLOAT4 & color)
{
    XMFLOAT3 corners[8];
    box.GetCorners(corners);
    return CreateFromCorners(corners, color);
}

Collision::WireFrameData Collision::CreateBoundingSphere(const DirectX::BoundingSphere & sphere, const DirectX::XMFLOAT4 & color, int slices)
{
    WireFrameData data;
    XMVECTOR center = XMLoadFloat3(&sphere.Center), posVec;
    XMFLOAT3 pos;
    float theta = 0.0f;
    for (int i = 0; i < slices; ++i)
    {
        posVec = XMVector3Transform(center + XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), XMMatrixRotationY(theta));
        XMStoreFloat3(&pos, posVec);
        data.vertexVec.push_back({ pos, color });
        posVec = XMVector3Transform(center + XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMMatrixRotationZ(theta));
        XMStoreFloat3(&pos, posVec);
        data.vertexVec.push_back({ pos, color });
        posVec = XMVector3Transform(center + XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f), XMMatrixRotationX(theta));
        XMStoreFloat3(&pos, posVec);
        data.vertexVec.push_back({ pos, color });
        theta += XM_2PI / slices;
    }
    for (int i = 0; i < slices; ++i)
    {
        data.indexVec.push_back(i * 3);
        data.indexVec.push_back((i + 1) % slices * 3);

        data.indexVec.push_back(i * 3 + 1);
        data.indexVec.push_back((i + 1) % slices * 3 + 1);

        data.indexVec.push_back(i * 3 + 2);
        data.indexVec.push_back((i + 1) % slices * 3 + 2);
    }


    return data;
}

Collision::WireFrameData Collision::CreateBoundingFrustum(const DirectX::BoundingFrustum & frustum, const DirectX::XMFLOAT4 & color)
{
    XMFLOAT3 corners[8];
    frustum.GetCorners(corners);
    return CreateFromCorners(corners, color);
}

std::vector<Transform> XM_CALLCONV Collision::FrustumCulling(
    const std::vector<Transform>& transforms, const DirectX::BoundingBox& localBox, DirectX::FXMMATRIX View, DirectX::CXMMATRIX Proj)
{
    std::vector<Transform> acceptedData;

    BoundingFrustum frustum;
    BoundingFrustum::CreateFromMatrix(frustum, Proj);

    BoundingOrientedBox localOrientedBox, orientedBox;
    BoundingOrientedBox::CreateFromBoundingBox(localOrientedBox, localBox);
    for (auto& t : transforms)
    {
        XMMATRIX W = t.GetLocalToWorldMatrixXM();
        // 将有向包围盒从局部坐标系变换到视锥体所在的局部坐标系(观察坐标系)中
        localOrientedBox.Transform(orientedBox, W * View);
        // 相交检测
        if (frustum.Intersects(orientedBox))
            acceptedData.push_back(t);
    }

    return acceptedData;
}

Collision::WireFrameData Collision::CreateFromCorners(const DirectX::XMFLOAT3(&corners)[8], const DirectX::XMFLOAT4 & color)
{
    WireFrameData data;
    // AABB/OBB顶点索引如下    视锥体顶点索引如下
    //     3_______2             4__________5
    //    /|      /|             |\        /|
    //  7/_|____6/ |             | \      / |
    //  |  |____|__|            7|_0\____/1_|6
    //  | /0    | /1              \ |    | /
    //  |/______|/                 \|____|/
    //  4       5                   3     2
    for (int i = 0; i < 8; ++i)
        data.vertexVec.push_back({ corners[i], color });
    for (int i = 0; i < 4; ++i)
    {
        data.indexVec.push_back(i);
        data.indexVec.push_back(i + 4);

        data.indexVec.push_back(i);
        data.indexVec.push_back((i + 1) % 4);

        data.indexVec.push_back(i + 4);
        data.indexVec.push_back((i + 1) % 4 + 4);
    }
    return data;
}
