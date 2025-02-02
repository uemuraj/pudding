#pragma once

#include <Windows.h>
#include <functional>
#include <memory>
#include <string_view>

enum FileAction : unsigned long
{
	None = 0,
	Added = FILE_ACTION_ADDED,
	Removed = FILE_ACTION_REMOVED,
	Modified = FILE_ACTION_MODIFIED,
	RenamedOldName = FILE_ACTION_RENAMED_OLD_NAME,
	RenamedNewName = FILE_ACTION_RENAMED_NEW_NAME,
};

using FileActionCallback = std::function<void(std::wstring_view name, FileAction action)>;

class DirectoryWatcherThreadWork;

class DirectoryWatcher
{
	std::unique_ptr<DirectoryWatcherThreadWork> m_work;

public:
	DirectoryWatcher(FileActionCallback callback, const wchar_t * path);
	DirectoryWatcher();
	~DirectoryWatcher() noexcept;

	DirectoryWatcher & operator=(DirectoryWatcher && other) noexcept;

	std::exception_ptr GetException() const noexcept;
};

inline unsigned long g_DirectoryWatcherThreadMax = 10;
