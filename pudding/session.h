#pragma once

#include <memory>
#include <string>

class CurrentSessionInformation;

namespace wts
{
	bool IsConsoleSession();
	bool IsRemoteSession();

	class Client
	{
		std::shared_ptr<CurrentSessionInformation> m_info;

	public:
		Client();
		~Client() noexcept = default;

		Client(const Client &) = default;
		Client & operator=(const Client &) = default;

		const wchar_t * HostName() const;
		const wchar_t * UserName() const;
	};
}
