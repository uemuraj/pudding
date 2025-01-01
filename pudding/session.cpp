#include "session.h"

#include <windows.h>
#include <wtsapi32.h>
#include <system_error>

//
// https://learn.microsoft.com/ja-jp/windows/win32/api/wtsapi32/ne-wtsapi32-wts_info_class
//

class CurrentSessionInformation
{
	void * m_buff;
	DWORD m_size;

public:
	CurrentSessionInformation(WTS_INFO_CLASS infoClass) : m_buff(nullptr), m_size(0)
	{
		if (!::WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, infoClass, (LPWSTR *) &m_buff, &m_size))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WTSQuerySessionInformation()");
		}
	}

	~CurrentSessionInformation()
	{
		::WTSFreeMemory(m_buff);
	}

	bool operator==(unsigned short value) const
	{
		return *(unsigned short *) m_buff == value;
	}

	const WTSCLIENTW & GetClient() const
	{
		return *(WTSCLIENTW *) m_buff;
	}
};

namespace wts
{
	bool IsConsoleSession()
	{
		CurrentSessionInformation info(WTSClientProtocolType);

		return info == USHORT(0);
	}

	bool IsRemoteSession()
	{
		CurrentSessionInformation info(WTSClientProtocolType);

		return info == USHORT(2);
	}

	Client::Client() : m_info(std::make_shared<CurrentSessionInformation>(WTSClientInfo))
	{}

	const wchar_t * Client::HostName() const
	{
		decltype(auto) client = m_info->GetClient();

		return client.ClientName;
	}

	const wchar_t * Client::UserName() const
	{
		decltype(auto) client = m_info->GetClient();

		return client.UserName;
	}
}
