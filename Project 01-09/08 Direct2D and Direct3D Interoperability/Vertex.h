//***************************************************************************************
// Vertex.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 定义了一些顶点结构体和输入布局
// Defines vertex structures and input layouts.
//***************************************************************************************

#ifndef VERTEX_H
#define VERTEX_H

#include <d3d11_1.h>
#include <DirectXMath.h>

struct VertexPos
{
    DirectX::XMFLOAT3 pos;
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[1];
};

struct VertexPosColor
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
};

struct VertexPosTex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 tex;
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
};

struct VertexPosSize
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 size;
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
};

struct VertexPosNormalColor
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT4 color;
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
};


struct VertexPosNormalTex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 tex;
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
};

struct VertexPosNormalTangentTex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT4 tangent;
    DirectX::XMFLOAT2 tex;
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[4];
};

#endif
