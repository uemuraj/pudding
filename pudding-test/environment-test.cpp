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

TEST(EnvironmentTest, GetEnvironmnetValuePtrSuccess)
{
	auto env = L"OS=Windows_NT\0PATHEXT=.COM;.EXE;.BAT;.CMD\0";

	EXPECT_STREQ(GetEnvironmnetValuePtr(env, L"OS"), L"Windows_NT");
	EXPECT_STREQ(GetEnvironmnetValuePtr(env, L"PATHEXT"), L".COM;.EXE;.BAT;.CMD");
}

TEST(EnvironmentTest, GetEnvironmnetValuePtrFailure)
{
	auto env = L"OS=Windows_NT\0PATHEXT=.COM;.EXE;.BAT;.CMD\0";

	EXPECT_EQ(nullptr, GetEnvironmnetValuePtr(env, L"OST"));
	EXPECT_EQ(nullptr, GetEnvironmnetValuePtr(env, L"PATH"));
}

TEST(EnvironmentTest, ExpandEnvironmentValueSuccess)
{
	auto env = L"ALLUSERSPROFILE=C:\\ProgramData\0USERPROFILE=C:\\Users\\uemur\0";

	EXPECT_STREQ(ExpandEnvironmentValue(env, L"%ALLUSERSPROFILE%\\Microsoft").c_str(), L"C:\\ProgramData\\Microsoft");
	EXPECT_STREQ(ExpandEnvironmentValue(env, L"%USERPROFILE%\\Desktop").c_str(), L"C:\\Users\\uemur\\Desktop");
}

TEST(EnvironmentTest, ExpandEnvironmentValueFailure)
{
	auto env = L"ALLUSERSPROFILE=C:\\ProgramData\0USERPROFILE=C:\\Users\\uemur\0";

	EXPECT_STREQ(ExpandEnvironmentValue(env, L"%ALLUSERSPROFILE\\Microsoft").c_str(), L"%ALLUSERSPROFILE\\Microsoft");
	EXPECT_STREQ(ExpandEnvironmentValue(env, L"ALLUSERSPROFILE%\\Microsoft").c_str(), L"ALLUSERSPROFILE%\\Microsoft");
	EXPECT_STREQ(ExpandEnvironmentValue(env, L"%LOCALAPPDATA%\\TEMP").c_str(), L"%LOCALAPPDATA%\\TEMP");
}
