//***************************************************************************************
// MeshData.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 存放材质与属性
// Material and property storage.
//***************************************************************************************


#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <string_view>
#include <unordered_map>
#include "XUtil.h"
#include "Property.h"

class Material
{
public:
    Material() = default;

    void Clear()
    {
        m_Properties.clear();
    }

    template<class T>
    void Set(std::string_view name, const T& value)
    {
        static_assert(IsVariantMember<T, Property>::value, "Type T isn't one of the Property types!");
        m_Properties[StringToID(name)] = value;
    }

    template<class T>
    const T& Get(std::string_view name) const
    {
        auto it = m_Properties.find(StringToID(name));
        return std::get<T>(it->second);
    }

    template<class T>
    T& Get(std::string_view name)
    {
        return const_cast<T&>(static_cast<const Material*>(this)->Get<T>(name));
    }

    template<class T>
    bool Has(std::string_view name) const
    {
        auto it = m_Properties.find(StringToID(name));
        if (it == m_Properties.end() || !std::holds_alternative<T>(it->second))
            return false;
        return true;
    }

    template<class T>
    const T* TryGet(std::string_view name) const
    {
        auto it = m_Properties.find(StringToID(name));
        if (it != m_Properties.end())
            return &std::get<T>(it->second);
        else
            return nullptr;
    }

    template<class T>
    T* TryGet(std::string_view name)
    {
        return const_cast<T*>(static_cast<const Material*>(this)->TryGet<T>(name));
    }

    bool HasProperty(std::string_view name) const
    {
        return m_Properties.find(StringToID(name)) != m_Properties.end();
    }

private:

    std::unordered_map<XID, Property> m_Properties;
};


#endif
