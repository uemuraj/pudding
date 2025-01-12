#pragma once

#include <memory>
#include <vector>
#include <string>

#include <string_view>
#include <unordered_map>

using Section = std::unordered_map<std::wstring, std::wstring>;
using Profile = std::unordered_map<std::wstring, Section>;

Profile LoadProfile(const std::wstring & path);


#include "utility.h"

inline decltype(auto) LoadProfile()
{
	auto path = GetCurrentModuleFileName();

	if (auto pos = path.rfind(L'.'); pos != std::wstring::npos)
	{
		path.resize(pos);
	}

	path += L".ini";

	return LoadProfile(path);
}
