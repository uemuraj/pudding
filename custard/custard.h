#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <filesystem>

namespace custard
{
	class SlackApi;

	class SlackBot
	{
		std::unique_ptr<SlackApi> m_api;
		std::wstring m_token;
		std::wstring m_name;
		std::filesystem::path m_icon;

	public:
		SlackBot(std::wstring_view token);
		~SlackBot() noexcept;

		bool Post(std::wstring_view channel, std::wstring_view message);

		const std::wstring & Name()
		{
			return m_name;
		}

		const std::filesystem::path & Icon()
		{
			return m_icon;
		}

	private:
		std::filesystem::path DownloadIcon(const std::wstring & url);
	};
}
