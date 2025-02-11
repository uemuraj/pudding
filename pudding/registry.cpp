#include "registry.h"

#include <memory>
#include <system_error>

struct RegInfo
{
	DWORD * pcSubKeys;
	DWORD * pcbMaxSubKeyLen;
	DWORD * pcbMaxClassLen;
	DWORD * pcValues;
	DWORD * pcchMaxValueNameLen;
	DWORD * pcbMaxValueLen;
	DWORD * pcbSecurityDescriptor;
	FILETIME * pftLastWriteTime;
};

void QueryRegInfo(HKEY key, const RegInfo & info)
{
	auto result = ::RegQueryInfoKeyW(key, nullptr, nullptr, nullptr, info.pcSubKeys, info.pcbMaxSubKeyLen, info.pcbMaxClassLen, info.pcValues, info.pcchMaxValueNameLen, info.pcbMaxValueLen, info.pcbSecurityDescriptor, info.pftLastWriteTime);

	if (result != ERROR_SUCCESS)
	{
		throw std::system_error(result, std::system_category(), "RegQueryInfoKeyW()");
	}
}

RegistryKey::RegistryKey(HKEY key, const wchar_t * subKey, REGSAM samDesired) : m_key(nullptr)
{
	if (auto result = ::RegOpenKeyExW(key, subKey, 0, samDesired, &m_key); result != ERROR_SUCCESS)
	{
		throw std::system_error(result, std::system_category(), "RegOpenKeyExW()");
	}
}

RegistryKey::~RegistryKey() noexcept
{
	if (m_key)
	{
		::RegCloseKey(m_key);
	}
}

RegistryKey::Iterator RegistryKey::begin() const
{
	return { m_key };
}

DWORD RegistryKey::end() const
{
	return size();
}

DWORD RegistryKey::size() const
{
	DWORD cValues{};
	QueryRegInfo(m_key, { .pcValues = &cValues });
	return cValues;
}

RegistryKey::Iterator::Iterator(HKEY key) : m_key(key), m_index(0), m_type(0), m_cchNameLen(0), m_cbValueLen(0)
{
	DWORD cchMaxNameLen{};
	DWORD cbMaxValueLen{};

	QueryRegInfo(key, { .pcchMaxValueNameLen = &cchMaxNameLen, .pcbMaxValueLen = &cbMaxValueLen });

	m_cchNameLenMax = cchMaxNameLen + 1;
	m_cbValueLenMax = cbMaxValueLen + sizeof(WCHAR);

	m_name.resize(m_cchNameLenMax);
	m_value.resize(m_cbValueLenMax);

	this->operator++();
}

RegistryKey::Iterator::~Iterator() noexcept
{
	// Do nothing
}

RegistryKey::Iterator & RegistryKey::Iterator::operator++()
{
	m_cchNameLen = m_cchNameLenMax;
	m_cbValueLen = m_cbValueLenMax;

	auto result = ::RegEnumValueW(m_key, m_index, m_name.data(), &m_cchNameLen, nullptr, &m_type, m_value.data(), &m_cbValueLen);

	switch (result)
	{
	case ERROR_SUCCESS:
		++m_index;
		return *this;

	case ERROR_NO_MORE_ITEMS:
		return *this;

	default:
		throw std::system_error(result, std::system_category(), "RegEnumValueW()");
	}
}

template<>
std::wstring_view RegistryValue::ToValue() const
{
	switch (Type)
	{
	case REG_SZ:
	case REG_EXPAND_SZ:
		if (!Data.empty())
		{
			std::wstring_view value{ (const wchar_t *) Data.data(), Data.size() / sizeof(wchar_t) };

			if (value.back() == L'\0')
			{
				value.remove_suffix(1);
			}

			return value;
		}
		break;

	default:
		throw std::invalid_argument("Unsupported registry value type");
	}

	return {};
}
