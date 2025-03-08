#include "pch.h"
#include <wconv.h>

TEST(CustardTest, ConvertFromWstring)
{
	std::wstring wstr = L"Hello, World!";
	std::u8string utf8 = ConvertFrom(wstr);
	EXPECT_STREQ((const char *) utf8.c_str(), "Hello, World!"); // !!
}

TEST(CustardTest, ConvertFromU8string)
{
	std::u8string utf8 = u8"Hello, World!";
	std::wstring wstr = ConvertFrom(utf8);
	EXPECT_STREQ(wstr.c_str(), L"Hello, World!");
}
