#include "pch.h"
#include <registry.h>

void PrintTo(const RegistryValue & value, std::ostream * os)
{
	PrintTo(value.Name, *os);
	*os << '=';
	PrintTo(value.ToValue<std::wstring_view>(), *os);
}

TEST(RegistryTest, SystemEnvironmentValues)
{
	RegistryKey values(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");

	for (const auto & value : values)
	{
		SUCCEED() << testing::PrintToString(value);
	}
}

TEST(RegistryTest, UserEnvironmentValues)
{
	RegistryKey values(HKEY_CURRENT_USER, L"Environment");

	for (const auto & value : values)
	{
		SUCCEED() << testing::PrintToString(value);
	}
}
