#pragma once

#include <memory>
#include <string_view>
#include <system_error>

class CustardContext;

class Custard
{
	std::unique_ptr<CustardContext> m_context;

public:
	Custard(std::wstring_view token);
	~Custard() noexcept;

	void PostToSlack(std::wstring_view channel, std::wstring_view message);
};
