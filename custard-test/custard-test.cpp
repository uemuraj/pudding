#include "pch.h"
#include <custard.h>

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

TEST(ParseJsonTest, SimpleObject)
{
	auto json = Json(LR"({"key": "value"})");

	auto [key, val] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(key.c_str(), L"key");

	auto value = std::get<std::wstring>(val.Parse());
	EXPECT_STREQ(value.c_str(), L"value");
}

TEST(ParseJsonTest, NestedObject)
{
	auto json = Json(LR"({"key": {"nested": "value"}})");

	auto [key, val] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(key.c_str(), L"key");

	auto nested = std::get<std::pair<std::wstring, Json>>(val.Parse());
	EXPECT_STREQ(nested.first.c_str(), L"nested");

	auto value = std::get<std::wstring>(nested.second.Parse());
	EXPECT_STREQ(value.c_str(), L"value");

	EXPECT_EQ(std::get<std::monostate>(json.Parse()), std::monostate());
}

TEST(ParseJsonTest, Array)
{
	auto json = Json(LR"(["one", "two", "three"])");

	auto one = std::get<Json>(json.Parse());
	EXPECT_STREQ(std::get<std::wstring>(one.Parse()).c_str(), L"one");

	auto two = std::get<Json>(json.Parse());
	EXPECT_STREQ(std::get<std::wstring>(two.Parse()).c_str(), L"two");

	auto three = std::get<Json>(json.Parse());
	EXPECT_STREQ(std::get<std::wstring>(three.Parse()).c_str(), L"three");

	EXPECT_EQ(std::get<std::monostate>(json.Parse()), std::monostate());
}

TEST(ParseJsonTest, NestedArray)
{
	auto json = Json(LR"([["one", "two"], ["three", "four"]])");

	auto array0 = std::get<Json>(json.Parse());
	EXPECT_STREQ(std::get<std::wstring>(std::get<Json>(array0.Parse()).Parse()).c_str(), L"one");
	EXPECT_STREQ(std::get<std::wstring>(std::get<Json>(array0.Parse()).Parse()).c_str(), L"two");
	EXPECT_EQ(std::get<std::monostate>(array0.Parse()), std::monostate());

	auto array1 = std::get<Json>(json.Parse());
	EXPECT_STREQ(std::get<std::wstring>(std::get<Json>(array1.Parse()).Parse()).c_str(), L"three");
	EXPECT_STREQ(std::get<std::wstring>(std::get<Json>(array1.Parse()).Parse()).c_str(), L"four");
	EXPECT_EQ(std::get<std::monostate>(array1.Parse()), std::monostate());
}
