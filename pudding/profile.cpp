#include "profile.h"
#include "utility.h"

#include <Windows.h>
#include <system_error>


std::vector<wchar_t> LoadSectionNames(const wchar_t * path)
{
	std::vector<wchar_t> buff(8);

	for (auto nsize = static_cast<DWORD>(buff.size()); nsize <= (32 * 1024); nsize += 1024)
	{
		buff.resize(nsize);

		if (::GetPrivateProfileSectionNamesW(buff.data(), nsize, path) < (nsize - 2))
		{
			break;
		}
	}

	return buff;
}

std::vector<wchar_t> LoadSectionData(const wchar_t * path, const wchar_t * name)
{
	std::vector<wchar_t> buff(8);

	for (auto nsize = static_cast<DWORD>(buff.size()); nsize <= (32 * 1024); nsize += 1024)
	{
		buff.resize(nsize);

		if (::GetPrivateProfileSectionW(name, buff.data(), nsize, path) < (nsize - 2))
		{
			break;
		}
	}

	return buff;
}

Profile LoadProfile(const std::wstring & path)
{
	Profile profile;

	auto sectionName = LoadSectionNames(path.c_str());

	for (auto name = sectionName.data(); *name; name += (wcslen(name) + 1))
	{
		auto sectionData = LoadSectionData(path.c_str(), name);

		auto & section = profile[name];

		for (auto line = sectionData.data(); *line; line += (wcslen(line) + 1))
		{
			if (auto pos = wcschr(line, L'='); pos)
			{
				section[std::wstring(line, pos - line)] = pos + 1;
			}
		}
	}

	return profile;
}
