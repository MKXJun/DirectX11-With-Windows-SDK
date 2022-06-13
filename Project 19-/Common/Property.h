
#pragma once

#ifndef PROPERTY_H
#define PROPERTY_H

#include <memory>
#include <variant>
#include <vector>
#include <string>
#include <DirectXMath.h>

template<class T, class V>
struct IsVariantMember;

template<class T, class... ALL_V>
struct IsVariantMember<T, std::variant<ALL_V...>> : public std::disjunction<std::is_same<T, ALL_V>...> {};

using Property = std::variant<
    int, uint32_t, float, DirectX::XMFLOAT2, DirectX::XMFLOAT3, DirectX::XMFLOAT4, DirectX::XMFLOAT4X4, 
    std::vector<float>, std::vector<DirectX::XMFLOAT4>, std::vector<DirectX::XMFLOAT4X4>,
    std::string>;

#endif
