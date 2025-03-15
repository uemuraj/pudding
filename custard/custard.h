#pragma once

#include <memory>
#include <filesystem>
#include <string>
#include <string_view>

class SlackWebApi;

class Custard
{
	std::unique_ptr<SlackWebApi> m_webapi;
	std::wstring m_token;

public:
	Custard(std::wstring_view token);
	~Custard() noexcept;

	bool Post(std::wstring_view channel, std::wstring_view message);

	std::wstring BotName();

	std::filesystem::path BotIcon();
};
