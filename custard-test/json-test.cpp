#include "pch.h"
#include <json.h>

#include <optional>

struct SlackResult
{
	std::optional<bool> ok;

	struct Message
	{
		std::wstring text;

		void operator()(std::monostate)
		{
			// do nothing
		}

		void operator()(Json & json)
		{
			while (!json.Empty())
			{
				auto parsed = json.Parse();
				std::visit(*this, parsed);
			}
		}

		void operator()(std::pair<std::wstring, Json> & pair)
		{
			auto & [key, value] = pair;
			if (key == L"text")
			{
				text = std::get<std::wstring>(value.Parse());
			}
		}

		void operator()(std::wstring & str)
		{
			// do nothing
		}
	};

	Message message;

	void operator()(std::monostate)
	{
		// do nothing
	}

	void operator()(Json & json)
	{
		while (!json.Empty())
		{
			auto parsed = json.Parse();
			std::visit(*this, parsed);
		}
	}

	void operator()(std::pair<std::wstring, Json> & pair)
	{
		auto & [key, value] = pair;
		if (key == L"ok")
		{
			ok = (std::get<std::wstring>(value.Parse()) == L"true");
		}
		else if (key == L"message")
		{
			auto parsed = value.Parse();
			std::visit(message, parsed);
		}
	}

	void operator()(std::wstring & str)
	{
		// do nothing
	}
};

TEST(ParseJsonTest, SlackResultOk)
{
	auto json = Json(LR"({"ok":true})");
	auto result = SlackResult();
	result(json);

	EXPECT_TRUE(result.ok.has_value());
	EXPECT_TRUE(result.ok.value());

	json = Json(LR"({"ok":false})");
	result = SlackResult();
	result(json);

	EXPECT_TRUE(result.ok.has_value());
	EXPECT_FALSE(result.ok.value());
}

TEST(ParseJsonTest, SlackResultMessage)
{
	auto json = Json(LR"({"message":{"text":"Hello, World!"}})");
	auto result = SlackResult();
	result.message(json);

	EXPECT_TRUE(result.message.text == L"Hello, World!");
}

TEST(ParseJsonTest, SlackResultMessageOk)
{
	auto json = Json(LR"({"ok":true,"message":{"text":"Hello, World!"}})");
	auto result = SlackResult();
	result(json);

	EXPECT_TRUE(result.ok.has_value());
	EXPECT_TRUE(result.ok.value());
	EXPECT_TRUE(result.message.text == L"Hello, World!");
}
