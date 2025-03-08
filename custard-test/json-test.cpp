#include "pch.h"
#include <json.h>
#include <optional>


TEST(ParseJsonTest, SimpleObject)
{
	auto json = Json(LR"({"key": "value"})");

	EXPECT_EQ(Json::State::Object, std::get<Json::State>(json.Parse()));

	auto [key, val] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(key.c_str(), L"key");
	EXPECT_STREQ(val.GetString().c_str(), L"value");

	EXPECT_EQ(Json::State::End, std::get<Json::State>(json.Parse()));
}

TEST(ParseJsonTest, NestedObject)
{
	auto json = Json(LR"({"key": {"nested": "value"}})");

	EXPECT_EQ(Json::State::Object, std::get<Json::State>(json.Parse()));

	auto [key, val] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(key.c_str(), L"key");

	EXPECT_EQ(Json::State::Object, std::get<Json::State>(json.Parse()));
	auto [nestedKey, nestedVal] = std::get<std::pair<std::wstring, Json>>(val.Parse());
	EXPECT_STREQ(nestedKey.c_str(), L"nested");
	EXPECT_STREQ(nestedVal.GetString().c_str(), L"value");
	EXPECT_EQ(Json::State::Next, std::get<Json::State>(json.Parse()));

	EXPECT_EQ(Json::State::End, std::get<Json::State>(json.Parse()));
}

TEST(ParseJsonTest, SimpleArray)
{
	auto json = Json(LR"(["one", "two", "three"])");

	EXPECT_EQ(Json::State::Array, std::get<Json::State>(json.Parse()));

	EXPECT_STREQ(json.GetString().c_str(), L"one");
	EXPECT_STREQ(json.GetString().c_str(), L"two");
	EXPECT_STREQ(json.GetString().c_str(), L"three");

	EXPECT_EQ(Json::State::End, std::get<Json::State>(json.Parse()));
}

TEST(ParseJsonTest, NestedArray)
{
	auto json = Json(LR"([["one", "two"], ["three", "four"]])");

	EXPECT_EQ(Json::State::Array, std::get<Json::State>(json.Parse()));

	EXPECT_EQ(Json::State::Array, std::get<Json::State>(json.Parse()));
	EXPECT_STREQ(json.GetString().c_str(), L"one");
	EXPECT_STREQ(json.GetString().c_str(), L"two");
	EXPECT_EQ(Json::State::Next, std::get<Json::State>(json.Parse()));

	EXPECT_EQ(Json::State::Array, std::get<Json::State>(json.Parse()));
	EXPECT_STREQ(json.GetString().c_str(), L"three");
	EXPECT_STREQ(json.GetString().c_str(), L"four");
	EXPECT_EQ(Json::State::Next, std::get<Json::State>(json.Parse()));

	EXPECT_EQ(Json::State::End, std::get<Json::State>(json.Parse()));
}


struct ResponseType1
{
	std::optional<bool> ok;
	std::wstring error;
	std::wstring warning;

	struct Metadata
	{
		struct Warnings : std::vector<std::wstring>
		{
			void operator()(std::wstring && value)
			{
				push_back(value);
			}
		};

		Warnings warnings;

		void operator()(std::wstring && key, Json && value)
		{
			if (key == L"warnings")
			{
				VisitJson(warnings, value);
			}
		}
	};

	Metadata metadata;

	void operator()(std::wstring && key, Json && value)
	{
		if (key == L"ok")
		{
			ok = value.GetBool();
			return;
		}

		if (key == L"error")
		{
			error = value.GetString();
			return;
		}

		if (key == L"warning")
		{
			warning = value.GetString();
			return;
		}

		if (key == L"response_metadata")
		{
			VisitJson(metadata, value);
			return;
		}
	}
};

TEST(ParseJsonTest, ResponseType1)
{
	auto json = Json(LR"({"ok":false,"error":"not_in_channel","warning":"superfluous_charset","response_metadata":{"warnings":["superfluous_charset"]}})");

	ResponseType1 response;
	VisitJson(response, json);

	EXPECT_FALSE(response.ok.value());
	EXPECT_STREQ(response.error.c_str(), L"not_in_channel");
	EXPECT_STREQ(response.warning.c_str(), L"superfluous_charset");

	ASSERT_FALSE(response.metadata.warnings.empty());
	EXPECT_STREQ(response.metadata.warnings[0].c_str(), L"superfluous_charset");
}

struct ResponseType2
{
	std::optional<bool> ok;

	struct Message
	{
		std::wstring type;
		std::wstring text;

		struct BotProfile
		{
			std::wstring name;

			struct Icons
			{
				std::wstring image36;
				std::wstring image48;
				std::wstring image72;

				void operator()(std::wstring && key, Json && value)
				{
					if (key == L"image_36")
					{
						image36 = value.GetString();
						return;
					}

					if (key == L"image_48")
					{
						image48 = value.GetString();
						return;
					}

					if (key == L"image_72")
					{
						image72 = value.GetString();
						return;
					}
				}
			};

			BotProfile::Icons icons;

			void operator()(std::wstring && key, Json && value)
			{
				if (key == L"name")
				{
					name = value.GetString();
					return;
				}

				if (key == L"icons")
				{
					VisitJson(icons, value);
					return;
				}
			}
		};

		BotProfile botProfile;

		void operator()(std::wstring && key, Json && value)
		{
			if (key == L"type")
			{
				type = value.GetString();
				return;
			}

			if (key == L"text")
			{
				text = value.GetString();
				return;
			}

			if (key == L"bot_profile")
			{
				VisitJson(botProfile, value);
				return;
			}
		}
	};

	Message message;

	void operator()(std::wstring && key, Json && value)
	{
		if (key == L"ok")
		{
			ok = value.GetBool();
			return;
		}

		if (key == L"message")
		{
			VisitJson(message, value);
			return;
		}
	}
};

TEST(ParseJsonTest, ResponseType2)
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

	ResponseType2 response;
	VisitJson(response, json);

	EXPECT_TRUE(response.ok.value());
	EXPECT_STREQ(response.message.type.c_str(), L"message");
	EXPECT_STREQ(response.message.text.c_str(), L"新しいチャンネルはどんな感じ？");
	EXPECT_STREQ(response.message.botProfile.name.c_str(), L"hoge");
	EXPECT_STREQ(response.message.botProfile.icons.image36.c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/bot_36.png");
	EXPECT_STREQ(response.message.botProfile.icons.image48.c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/bot_48.png");
	EXPECT_STREQ(response.message.botProfile.icons.image72.c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/service_72.png");
}
