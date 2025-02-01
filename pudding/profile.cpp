#include "profile.h"
#include "utility.h"

#include <Windows.h>
#include <stdexcept>
#include <vector>

static std::vector<wchar_t> LoadSectionNames(const wchar_t * file)
{
	std::vector<wchar_t> buff(8);

	for (auto nsize = (DWORD) buff.size(); nsize <= (32 * 1024); nsize += 1024)
	{
		buff.resize(nsize);

		if (::GetPrivateProfileSectionNamesW(buff.data(), nsize, file) < (nsize - 2))
		{
			return buff;
		}
	}

	throw std::overflow_error("Buffer overflow while loading section names");
}

static std::vector<wchar_t> LoadSectionData(const wchar_t * file, const wchar_t * name)
{
	std::vector<wchar_t> buff(8);

	for (auto nsize = (DWORD) buff.size(); nsize <= (32 * 1024); nsize += 1024)
	{
		buff.resize(nsize);

		if (::GetPrivateProfileSectionW(name, buff.data(), nsize, file) < (nsize - 2))
		{
			return buff;
		}
	}

	throw std::overflow_error("Buffer overflow while loading section names");
}

std::unique_ptr<Profile> LoadProfile(const std::wstring & file)
{
	auto profile = std::make_unique<Profile>();

	auto sectionName = LoadSectionNames(file.c_str());

	for (auto name = sectionName.data(); *name; name += (wcslen(name) + 1))
	{
		auto sectionData = LoadSectionData(file.c_str(), name);

		auto & section = profile->operator[](name);

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

std::unique_ptr<Profile> LoadProfile()
{
	auto file = GetCurrentModuleFileName().ReplaceExtension(L".ini");

	return LoadProfile(file);
}
