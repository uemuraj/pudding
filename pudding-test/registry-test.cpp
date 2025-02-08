#include "pch.h"
#include <registry.h>

TEST(RegistryTest, SystemEnvironmentValues)
{
	RegistryValues values(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");

	for (const auto & value : values)
	{
		std::wostringstream os;

		os << value.Name << L" = " << value.Value<std::wstring_view>() << std::endl;

		::OutputDebugStringW(os.str().c_str());
	}
}

TEST(RegistryTest, UserEnvironmentValues)
{
	RegistryValues values(HKEY_CURRENT_USER, L"Environment");

	for (const auto & value : values)
	{
		std::wostringstream os;

		os << value.Name << L" = " << value.Value<std::wstring_view>() << std::endl;

		::OutputDebugStringW(os.str().c_str());
	}
}
