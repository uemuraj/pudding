#include "pch.h"

#include <json.h>
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

TEST(ParseJsonTest, SimpleObject)
{
	auto json = Json(LR"({"key": "value"})");
	EXPECT_TRUE(json.IsObject());

	auto [key, val] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(key.c_str(), L"key");
	EXPECT_FALSE(val.IsObject() || val.IsArray());

	auto value = std::get<std::wstring>(val.Parse());
	EXPECT_STREQ(value.c_str(), L"value");
}

TEST(ParseJsonTest, NestedObject)
{
	auto json = Json(LR"({"key": {"nested": "value"}})");
	EXPECT_TRUE(json.IsObject());

	auto [key, val] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(key.c_str(), L"key");
	EXPECT_TRUE(val.IsObject());

	auto [nestedKey, nestedVal] = std::get<std::pair<std::wstring, Json>>(val.Parse());
	EXPECT_STREQ(nestedKey.c_str(), L"nested");
	EXPECT_FALSE(nestedVal.IsObject() || nestedVal.IsArray());

	auto value = std::get<std::wstring>(nestedVal.Parse());
	EXPECT_STREQ(value.c_str(), L"value");

	EXPECT_EQ(std::get<std::monostate>(json.Parse()), std::monostate());
}

TEST(ParseJsonTest, Array)
{
	auto json = Json(LR"(["one", "two", "three"])");
	EXPECT_TRUE(json.IsArray());

	auto one = std::get<Json>(json.Parse());
	EXPECT_FALSE(one.IsObject() || one.IsArray());
	EXPECT_STREQ(std::get<std::wstring>(one.Parse()).c_str(), L"one");

	auto two = std::get<Json>(json.Parse());
	EXPECT_FALSE(two.IsObject() || two.IsArray());
	EXPECT_STREQ(std::get<std::wstring>(two.Parse()).c_str(), L"two");

	auto three = std::get<Json>(json.Parse());
	EXPECT_FALSE(three.IsObject() || three.IsArray());
	EXPECT_STREQ(std::get<std::wstring>(three.Parse()).c_str(), L"three");

	EXPECT_EQ(std::get<std::monostate>(json.Parse()), std::monostate());
}

TEST(ParseJsonTest, NestedArray)
{
	auto json = Json(LR"([["one", "two"], ["three", "four"]])");
	EXPECT_TRUE(json.IsArray());

	auto array0 = std::get<Json>(json.Parse());
	EXPECT_TRUE(array0.IsArray());
	EXPECT_STREQ(std::get<std::wstring>(std::get<Json>(array0.Parse()).Parse()).c_str(), L"one");
	EXPECT_STREQ(std::get<std::wstring>(std::get<Json>(array0.Parse()).Parse()).c_str(), L"two");
	EXPECT_EQ(std::get<std::monostate>(array0.Parse()), std::monostate());

	auto array1 = std::get<Json>(json.Parse());
	EXPECT_TRUE(array1.IsArray());
	EXPECT_STREQ(std::get<std::wstring>(std::get<Json>(array1.Parse()).Parse()).c_str(), L"three");
	EXPECT_STREQ(std::get<std::wstring>(std::get<Json>(array1.Parse()).Parse()).c_str(), L"four");
	EXPECT_EQ(std::get<std::monostate>(array1.Parse()), std::monostate());
}

TEST(ParseJsonTest, SampleResponse1)
{
	auto json = Json(LR"({"ok":false,"error":"not_in_channel","warning":"superfluous_charset","response_metadata":{"warnings":["superfluous_charset"]}})");

	auto [ok, result] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(ok.c_str(), L"ok");
	EXPECT_STREQ(std::get<std::wstring>(result.Parse()).c_str(), L"false");

	auto [error, errorValue] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(error.c_str(), L"error");
	EXPECT_STREQ(std::get<std::wstring>(errorValue.Parse()).c_str(), L"not_in_channel");

	auto [warning, warningValue] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(warning.c_str(), L"warning");
	EXPECT_STREQ(std::get<std::wstring>(warningValue.Parse()).c_str(), L"superfluous_charset");

	auto [response_metadata, response_metadataValue] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(response_metadata.c_str(), L"response_metadata");

	auto [warnings, warningsValue] = std::get<std::pair<std::wstring, Json>>(response_metadataValue.Parse());
	EXPECT_STREQ(warnings.c_str(), L"warnings");

	auto array = std::get<Json>(warningsValue.Parse());
	EXPECT_STREQ(std::get<std::wstring>(array.Parse()).c_str(), L"superfluous_charset");
	EXPECT_EQ(std::get<std::monostate>(array.Parse()), std::monostate());
}

TEST(ParseJsonTest, SampleResponse2)
{
	auto json = Json(LR"(
{
  "ok":true,
  "message":
  {
    "type":"message",
    "text":"\u65b0\u3057\u3044\u30c1\u30e3\u30f3\u30cd\u30eb\u306f\u3069\u3093\u306a\u611f\u3058\uff1f",
    "bot_profile":
    {
      "name":"hoge",
      "icons":
      {
        "image_36":"https:\/\/a.slack-edge.com\/hoge\/img\/plugins\/app\/bot_36.png",
        "image_48":"https:\/\/a.slack-edge.com\/hoge\/img\/plugins\/app\/bot_48.png",
        "image_72":"https:\/\/a.slack-edge.com\/hoge\/img\/plugins\/app\/service_72.png"}
      }
    }
  }
}
)");
	auto [ok, result] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(ok.c_str(), L"ok");
	EXPECT_STREQ(std::get<std::wstring>(result.Parse()).c_str(), L"true");

	auto [message, messageValue] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(message.c_str(), L"message");

	auto [type, typeValue] = std::get<std::pair<std::wstring, Json>>(messageValue.Parse());
	EXPECT_STREQ(type.c_str(), L"type");
	EXPECT_STREQ(std::get<std::wstring>(typeValue.Parse()).c_str(), L"message");

	auto [text, textValue] = std::get<std::pair<std::wstring, Json>>(messageValue.Parse());
	EXPECT_STREQ(text.c_str(), L"text");
	EXPECT_STREQ(std::get<std::wstring>(textValue.Parse()).c_str(), L"新しいチャンネルはどんな感じ？");

	auto [bot_profile, bot_profileValue] = std::get<std::pair<std::wstring, Json>>(messageValue.Parse());
	EXPECT_STREQ(bot_profile.c_str(), L"bot_profile");

	auto [name, nameValue] = std::get<std::pair<std::wstring, Json>>(bot_profileValue.Parse());
	EXPECT_STREQ(name.c_str(), L"name");
	EXPECT_STREQ(std::get<std::wstring>(nameValue.Parse()).c_str(), L"hoge");

	auto [icons, iconsValue] = std::get<std::pair<std::wstring, Json>>(bot_profileValue.Parse());
	EXPECT_STREQ(icons.c_str(), L"icons");

	auto [image_36, image_36Value] = std::get<std::pair<std::wstring, Json>>(iconsValue.Parse());
	EXPECT_STREQ(image_36.c_str(), L"image_36");
	EXPECT_STREQ(std::get<std::wstring>(image_36Value.Parse()).c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/bot_36.png");

	auto [image_48, image_48Value] = std::get<std::pair<std::wstring, Json>>(iconsValue.Parse());
	EXPECT_STREQ(image_48.c_str(), L"image_48");
	EXPECT_STREQ(std::get<std::wstring>(image_48Value.Parse()).c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/bot_48.png");

	auto [image_72, image_72Value] = std::get<std::pair<std::wstring, Json>>(iconsValue.Parse());
	EXPECT_STREQ(image_72.c_str(), L"image_72");
	EXPECT_STREQ(std::get<std::wstring>(image_72Value.Parse()).c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/service_72.png");
}
