#pragma once

#include <memory>
#include <string>
#include <unordered_map>

using Section = std::unordered_map<std::wstring, std::wstring>;
using Profile = std::unordered_map<std::wstring, Section>;

std::unique_ptr<Profile> LoadProfile(const std::wstring & file);
std::unique_ptr<Profile> LoadProfile();
