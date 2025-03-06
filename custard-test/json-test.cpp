#include "pch.h"
#include <json.h>
#include <optional>


TEST(ParseJsonTest, SimpleObject)
{
	auto json = Json(LR"({"key": "value"})");

	EXPECT_EQ(Json::State::Object, std::get<Json::State>(json.Parse()));

	auto [key, val] = std::get<std::pair<std::wstring, Json>>(json.Parse());
	EXPECT_STREQ(key.c_str(), L"key");
	EXPECT_STREQ(std::get<std::wstring>(val.Parse()).c_str(), L"value");

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
	EXPECT_STREQ(std::get<std::wstring>(nestedVal.Parse()).c_str(), L"value");
	EXPECT_EQ(Json::State::Next, std::get<Json::State>(json.Parse()));

	EXPECT_EQ(Json::State::End, std::get<Json::State>(json.Parse()));
}

TEST(ParseJsonTest, SimpleArray)
{
	auto json = Json(LR"(["one", "two", "three"])");

	EXPECT_EQ(Json::State::Array, std::get<Json::State>(json.Parse()));

	EXPECT_STREQ(std::get<std::wstring>(json.Parse()).c_str(), L"one");
	EXPECT_STREQ(std::get<std::wstring>(json.Parse()).c_str(), L"two");
	EXPECT_STREQ(std::get<std::wstring>(json.Parse()).c_str(), L"three");

	EXPECT_EQ(Json::State::End, std::get<Json::State>(json.Parse()));
}

TEST(ParseJsonTest, NestedArray)
{
	auto json = Json(LR"([["one", "two"], ["three", "four"]])");

	EXPECT_EQ(Json::State::Array, std::get<Json::State>(json.Parse()));

	EXPECT_EQ(Json::State::Array, std::get<Json::State>(json.Parse()));
	EXPECT_STREQ(std::get<std::wstring>(json.Parse()).c_str(), L"one");
	EXPECT_STREQ(std::get<std::wstring>(json.Parse()).c_str(), L"two");
	EXPECT_EQ(Json::State::Next, std::get<Json::State>(json.Parse()));

	EXPECT_EQ(Json::State::Array, std::get<Json::State>(json.Parse()));
	EXPECT_STREQ(std::get<std::wstring>(json.Parse()).c_str(), L"three");
	EXPECT_STREQ(std::get<std::wstring>(json.Parse()).c_str(), L"four");
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
		std::vector<std::wstring> warnings;

		bool operator()(std::pair<std::wstring, Json> && keyValue)
		{
			auto & [key, value] = keyValue;

			if (key == L"warnings")
			{
				while (std::visit(*this, value.Parse())) /**/;
			}

			return true;
		}

		bool operator()(Json && value)
		{
			return std::visit(*this, value.Parse());
		}

		bool operator()(std::wstring && value)
		{
			warnings.push_back(value);
			return true;
		}

		bool operator()(Json::State state)
		{
			return (state != Json::State::End);
		}
	};

	Metadata metadata;

	bool operator()(std::pair<std::wstring, Json> && keyValue)
	{
		auto & [key, value] = keyValue;

		if (key == L"ok")
		{
			ok = (std::get<std::wstring>(value.Parse()) == L"true");
			return true;
		}

		if (key == L"error")
		{
			error = std::get<std::wstring>(value.Parse());
			return true;
		}

		if (key == L"warning")
		{
			warning = std::get<std::wstring>(value.Parse());
			return true;
		}

		if (key == L"response_metadata")
		{
			while (std::visit(metadata, value.Parse())) /**/;
			return true;
		}

		return true;
	}

	bool operator()(Json && value)
	{
		return std::visit(*this, value.Parse());
	}

	bool operator()(std::wstring && value)
	{
		return true;
	}

	bool operator()(Json::State state)
	{
		return (state != Json::State::End);
	}
};

TEST(ParseJsonTest, ResponseType1)
{
	auto json = Json(LR"({"ok":false,"error":"not_in_channel","warning":"superfluous_charset","response_metadata":{"warnings":["superfluous_charset"]}})");

	ResponseType1 response;

	while (std::visit(response, json.Parse())) /**/;

	EXPECT_FALSE(response.ok.value());
	EXPECT_STREQ(response.error.c_str(), L"not_in_channel");
	EXPECT_STREQ(response.warning.c_str(), L"superfluous_charset");

	ASSERT_EQ(response.metadata.warnings.size(), 1);
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

				bool operator()(std::pair<std::wstring, Json> && keyValue)
				{
					auto & [key, value] = keyValue;

					if (key == L"image_36")
					{
						image36 = std::get<std::wstring>(value.Parse());
						return true;
					}

					if (key == L"image_48")
					{
						image48 = std::get<std::wstring>(value.Parse());
						return true;
					}

					if (key == L"image_72")
					{
						image72 = std::get<std::wstring>(value.Parse());
						return true;
					}

					return true;
				}

				bool operator()(Json && value)
				{
					return std::visit(*this, value.Parse());
				}

				bool operator()(std::wstring && value)
				{
					return true;
				}

				bool operator()(Json::State state)
				{
					return (state != Json::State::End);
				}
			};

			BotProfile::Icons icons;

			bool operator()(std::pair<std::wstring, Json> && keyValue)
			{
				auto & [key, value] = keyValue;

				if (key == L"name")
				{
					name = std::get<std::wstring>(value.Parse());
					return true;
				}

				if (key == L"icons")
				{
					while (std::visit(icons, value.Parse())) /**/;
					return true;
				}

				return true;
			}

			bool operator()(Json && value)
			{
				return std::visit(*this, value.Parse());
			}

			bool operator()(std::wstring && value)
			{
				return true;
			}

			bool operator()(Json::State state)
			{
				return (state != Json::State::End);
			}
		};

		BotProfile botProfile;

		bool operator()(std::pair<std::wstring, Json> && keyValue)
		{
			auto & [key, value] = keyValue;

			if (key == L"type")
			{
				type = std::get<std::wstring>(value.Parse());
				return true;
			}

			if (key == L"text")
			{
				text = std::get<std::wstring>(value.Parse());
				return true;
			}

			if (key == L"bot_profile")
			{
				while (std::visit(botProfile, value.Parse())) /**/;
				return true;
			}

			return true;
		}

		bool operator()(Json && value)
		{
			return std::visit(*this, value.Parse());
		}

		bool operator()(std::wstring && value)
		{
			return true;
		}

		bool operator()(Json::State state)
		{
			return (state != Json::State::End);
		}
	};

	Message message;

	bool operator()(std::pair<std::wstring, Json> && keyValue)
	{
		auto & [key, value] = keyValue;

		if (key == L"ok")
		{
			ok = (std::get<std::wstring>(value.Parse()) == L"true");
			return true;
		}

		if (key == L"message")
		{
			while (std::visit(message, value.Parse())) /**/;
			return true;
		}

		return true;
	}

	bool operator()(Json && value)
	{
		return std::visit(*this, value.Parse());
	}

	bool operator()(std::wstring && value)
	{
		return true;
	}

	bool operator()(Json::State state)
	{
		return (state != Json::State::End);
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

	while (std::visit(response, json.Parse())) /**/;

	EXPECT_TRUE(response.ok.value());
	EXPECT_STREQ(response.message.type.c_str(), L"message");
	EXPECT_STREQ(response.message.text.c_str(), L"新しいチャンネルはどんな感じ？");
	EXPECT_STREQ(response.message.botProfile.name.c_str(), L"hoge");
	EXPECT_STREQ(response.message.botProfile.icons.image36.c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/bot_36.png");
	EXPECT_STREQ(response.message.botProfile.icons.image48.c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/bot_48.png");
	EXPECT_STREQ(response.message.botProfile.icons.image72.c_str(), L"https://a.slack-edge.com/hoge/img/plugins/app/service_72.png");
}
