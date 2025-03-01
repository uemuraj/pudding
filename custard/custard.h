#pragma once

#include <memory>
#include <string>
#include <string_view>

class CustardContext;

class Custard
{
	std::unique_ptr<CustardContext> m_context;

public:
	Custard(std::wstring_view token);
	~Custard() noexcept;

	bool PostToSlack(std::wstring_view channel, std::wstring_view message);
};
