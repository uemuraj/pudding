#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <string_view>

class RegistryKey
{
	HKEY m_key;

public:
	RegistryKey(HKEY key, const wchar_t * subKey, REGSAM samDesired = KEY_READ);
	~RegistryKey() noexcept;

	RegistryKey(const RegistryKey &) = delete;
	RegistryKey & operator=(const RegistryKey &) = delete;

	class Iterator;

	Iterator begin() const;
	DWORD end() const;
	DWORD size() const;
};

struct RegistryValue
{
	DWORD Type;
	std::wstring_view Name;
	std::basic_string_view<unsigned char> Data;

	template<typename T>
	T ToValue() const
	{
		static_assert(false);
	}

	template<>
	std::wstring_view ToValue() const;
};

class RegistryKey::Iterator
{
	HKEY m_key;
	DWORD m_index;
	DWORD m_type;
	DWORD m_cchNameLen;
	DWORD m_cbValueLen;
	DWORD m_cchNameLenMax;
	DWORD m_cbValueLenMax;

	std::vector<wchar_t> m_name;
	std::vector<unsigned char> m_value;

public:
	Iterator(const Iterator &) = delete;
	Iterator(HKEY key);
	~Iterator() noexcept;

	Iterator & operator=(const Iterator &) = delete;
	Iterator & operator++();

	bool operator!=(DWORD index) const
	{
		return m_index != index;
	}

	RegistryValue operator*() const
	{
		return { m_type, { m_name.data(), m_cchNameLen }, { m_value.data(), m_cbValueLen } };
	}
};
