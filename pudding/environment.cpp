#include "environment.h"
#include "registry.h"

#include <ranges>
#include <utility>

std::vector<wchar_t> NewEnvironmentStrings(const EnvironmentStringsView & view)
{
	std::vector<wchar_t> env;

	if (!view.empty())
	{
		size_t size = 0;

		for (auto & [key, value] : view)
		{
			size += key.size();
			++size;
			size += value.size();
			++size;
		}

		env.resize(size + 1);

		auto buf = env.data();

		for (const auto & [key, value] : view)
		{
			buf = std::copy(key.begin(), key.end(), buf);
			*buf++ = L'=';
			buf = std::copy(value.begin(), value.end(), buf);
			++buf;
		}
	}

	return env;
}

class AllEnvironmentStringsView : public EnvironmentStringsView
{
	CurentUserEnvironment m_env0;
	VolatileUserEnvironment m_env1;
	CurrentSystemEnvironment m_env2;
	CurrentProcessEnvironment m_env3;
	std::wstring m_path;

public:
	AllEnvironmentStringsView()
	{
		merge(m_env0);
		merge(m_env1);
		merge(m_env2);
		merge(m_env3);

		m_path = this->operator[](L"Path");

		if (auto path = m_env2.find(L"Path"); path != m_env2.end())
		{
			if (m_path.empty())
			{
				m_path = path->second;
			}
			else if (m_path.back() == L';')
			{
				m_path += path->second;
			}
			else
			{
				m_path += L';';
				m_path += path->second;
			}
		}

		this->operator[](L"Path") = m_path;
	}
};

std::vector<wchar_t> NewEnvironmentStrings()
{
	return NewEnvironmentStrings(AllEnvironmentStringsView{});
}

const wchar_t * GetEnvironmnetValuePtr(const wchar_t * env, std::wstring_view name)
{
	const auto len = name.size();

	for (; *env; env += (wcslen(env) + 1))
	{
		if (env[len] == L'=')
		{
			if (_wcsnicmp(env, name.data(), len) == 0)
			{
				return env + len + 1;
			}
		}
	}

	return nullptr;
}

std::wstring ExpandEnvironmentValue(const wchar_t * env, std::wstring_view str)
{
	std::wstring buf;

	buf.reserve(str.size());

	for (auto src = str.data(); *src;)
	{
		if (*src != L'%')
		{
			buf.push_back(*src++);
			continue;
		}

		const wchar_t * end = src + 1;

		while (*end && *end != L'%')
		{
			++end;
		}

		if (*end != L'%')
		{
			buf.append(src, end - src);
			break;
		}

		const wchar_t * name = src + 1;
		const wchar_t * next = end + 1;

		if (auto value = GetEnvironmnetValuePtr(env, { name, (size_t) (end - name) }); value)
		{
			buf.append(value);
		}
		else
		{
			buf.append(src, next - src);
		}

		src = next;
	}

	return buf;
}


CurrentProcessEnvironment::CurrentProcessEnvironment() : m_env(::GetEnvironmentStringsW())
{
	if (auto msz = m_env; msz)
	{
		do
		{
			const wchar_t * name = msz;
			const wchar_t * value = wcschr(msz, L'=');

			if (value)
			{
				emplace(std::wstring_view{ name, (size_t) (value - name) }, value + 1);
			}

			msz += (wcslen(msz) + 1);
		}
		while (*msz);
	}
}

CurrentProcessEnvironment::~CurrentProcessEnvironment() noexcept
{
	if (m_env)
	{
		::FreeEnvironmentStringsW(m_env);
	}
}

CurrentProcessEnvironment::CurrentProcessEnvironment(CurrentProcessEnvironment && other) noexcept
	: EnvironmentStringsView(std::move(other)), m_env(std::exchange(other.m_env, nullptr))
{}


EnvironmentStrings::EnvironmentStrings(HKEY hKey, const wchar_t * subKey)
{
	for (const auto & entry : RegistryKey{ hKey, subKey })
	{
		if (entry.Type == REG_SZ || entry.Type == REG_EXPAND_SZ)
		{
			auto key = entry.Name;
			auto value = entry.ToValue<std::wstring_view>();
			emplace(key, value);
		}
	}
}

CurentUserEnvironment::CurentUserEnvironment() : m_env(HKEY_CURRENT_USER, L"Environment")
{
	for (const auto & [key, value] : m_env)
	{
		emplace(key, value);
	}
}

VolatileUserEnvironment::VolatileUserEnvironment() : m_env(HKEY_CURRENT_USER, L"Volatile Environment")
{
	for (const auto & [key, value] : m_env)
	{
		emplace(key, value);
	}
}

CurrentSystemEnvironment::CurrentSystemEnvironment()
	: m_env(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment")
{
	for (const auto & [key, value] : m_env)
	{
		emplace(key, value);
	}
}
