
#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <variant>
#include <string_view>
#include <unordered_map>
#include <DirectXMath.h>
#include "Property.h"

class Material
{
public:
	Material() = default;

	template<class T>
	void Set(std::string_view name, const T& value)
	{
		static_assert(IsVariantMember<T, Property>::value, "Type T isn't one of the Property types!");
		m_Properties[StringToID(name)] = value;
	}

	template<class T>
	const T* Get(std::string_view name) const
	{
		auto it = m_Properties.find(StringToID(name));
		if (it == m_Properties.end() || !std::holds_alternative<T>(it->second))
			return nullptr;
		return &std::get<T>(it->second);
	}

	const Property* Get(std::string_view name) const
	{
		auto it = m_Properties.find(StringToID(name));
		if (it == m_Properties.end())
			return nullptr;
		return &it->second;
	}

	bool HasProperty(std::string_view name) const
	{
		return m_Properties.find(StringToID(name)) != m_Properties.end();
	}

	void SetTexture(std::string_view name, std::string texName)
	{
		m_Textures[StringToID(name)] = std::move(texName);
	}

	bool HasTexture(std::string_view name) const
	{
		return m_Textures.find(StringToID(name)) != m_Textures.end();
	}

	const std::string& GetTexture(std::string_view name) const
	{
		auto it = m_Textures.find(StringToID(name));
		static std::string empty{};
		return it != m_Textures.end() ? it->second : empty;
	}

private:
	static size_t StringToID(std::string_view str)
	{
		static std::hash<std::string_view> hash;
		return hash(str);
	}
private:

	std::unordered_map<size_t, std::string> m_Textures;
	std::unordered_map<size_t, Property> m_Properties;
};


#endif
