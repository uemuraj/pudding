#include "pch.h"
#include <environment.h>

void PrintTo(const std::pair<std::wstring_view, std::wstring_view> & value, std::ostream * os)
{
	PrintTo(value.first, *os);
	*os << '=';
	PrintTo(value.second, *os);
}

void PrintTo(const EnvironmentStringsView & value, std::ostream * os)
{
	*os << '{';
	for (const auto & entry : value)
	{
		PrintTo(entry, os);
		*os << ',';
	}
	*os << '}';
}

TEST(EnvironmentTest, CurrentProcessEnvironmentValues)
{
	CurrentProcessEnvironment env;
	SUCCEED() << testing::PrintToString(env);
}

TEST(EnvironmentTest, CurentUserEnvironmentValues)
{
	CurentUserEnvironment env;
	SUCCEED() << testing::PrintToString(env);
}

TEST(EnvironmentTest, VolatileUserEnvironmentValues)
{
	VolatileUserEnvironment env;
	SUCCEED() << testing::PrintToString(env);
}

TEST(EnvironmentTest, CurrentSystemEnvironmentValues)
{
	CurrentSystemEnvironment env;
	SUCCEED() << testing::PrintToString(env);
}

TEST(EnvironmentTest, NewEnvironmentStrings)
{
	auto env = NewEnvironmentStrings();
	auto msz = env.data();

	ASSERT_NE(nullptr, msz);

	while (*msz)
	{
		SUCCEED() << testing::PrintToString(std::wstring_view{ msz });

		msz += (wcslen(msz) + 1);
	}
}
