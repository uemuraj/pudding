#include "dwatch.h"
#include "thread.h"

#include <exception>
#include <system_error>


struct DirectoryWatcherThreadPool : ThreadpoolEnviroment
{
	DirectoryWatcherThreadPool() : ThreadpoolEnviroment(true, 1, 10)
	{}
};

static std::shared_ptr<DirectoryWatcherThreadPool> GetDirectoryWatcherThreadPool()
{
	static std::weak_ptr<DirectoryWatcherThreadPool> g_directoryWatcherThreadPool;

	static CriticalSection g_criticalSection;

	LockGuard lock(g_criticalSection);

	auto threadPool = g_directoryWatcherThreadPool.lock();

	if (!threadPool)
	{
		threadPool = std::make_shared<DirectoryWatcherThreadPool>();

		g_directoryWatcherThreadPool = threadPool;
	}

	return threadPool;
}


class DirectoryHandle
{
	static constexpr DWORD accessMode = FILE_LIST_DIRECTORY;
	static constexpr DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	static constexpr DWORD creationDisposition = OPEN_EXISTING;
	static constexpr DWORD flagsAndAttributes = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;

	HANDLE m_handle;

public:
	DirectoryHandle(const wchar_t * path) : m_handle(::CreateFile(path, accessMode, shareMode, nullptr, creationDisposition, flagsAndAttributes, nullptr))
	{
		if (m_handle == INVALID_HANDLE_VALUE)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "CreateFile()");
		}
	}

	~DirectoryHandle() noexcept
	{
		::CloseHandle(m_handle);
	}

	operator HANDLE() const noexcept
	{
		return m_handle;
	}
};

class DirectoryChanges
{
	DirectoryHandle m_handle;
	StopEvent m_complete;
	StopEvent m_cancel;

	OVERLAPPED m_overlapped;
	BYTE m_buff[64 * 1024];

public:
	DirectoryChanges(const wchar_t * path) : m_handle(path), m_overlapped{ .hEvent = m_complete }, m_buff{}
	{}

	void Cancel() noexcept
	{
		::SetEvent(m_cancel);
	}

	void Watch(FileActionCallback callback)
	{
		constexpr DWORD notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;

		HANDLE handles[] = { m_complete, m_cancel };

		while (::ReadDirectoryChangesW(m_handle, m_buff, _countof(m_buff), false, notifyFilter, nullptr, &m_overlapped, nullptr))
		{
			switch (::WaitForMultipleObjects(_countof(handles), handles, false, INFINITE))
			{
			case WAIT_OBJECT_0:
				GetReadDirectoryChangesResult(callback);
				continue;

			case WAIT_OBJECT_0 + 1:
				return;

			default:
				throw std::system_error(::GetLastError(), std::system_category(), "WaitForMultipleObjects()");
			}
		}

		throw std::system_error(::GetLastError(), std::system_category(), "ReadDirectoryChangesW()");
	}

private:
	void GetReadDirectoryChangesResult(FileActionCallback callback)
	{
		DWORD bytesTransferred(0);

		if (!::GetOverlappedResult(m_handle, &m_overlapped, &bytesTransferred, false))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "GetOverlappedResult()");
		}

		if (bytesTransferred == 0)
		{
			throw std::system_error(ERROR_HANDLE_EOF, std::system_category(), "ReadDirectoryChangesW()");
		}

		if (bytesTransferred < sizeof(FILE_NOTIFY_INFORMATION))
		{
			throw std::system_error(ERROR_INVALID_DATA, std::system_category(), "ReadDirectoryChangesW()");
		}

		auto buff = m_buff;

		DWORD limit = (bytesTransferred - sizeof(FILE_NOTIFY_INFORMATION));

		for (DWORD offset = 0; offset <= limit; buff += offset)
		{
			offset = Callback(callback, (FILE_NOTIFY_INFORMATION *) buff);

			if (offset == 0)
			{
				break;
			}
		}
	}

	DWORD Callback(FileActionCallback callback, FILE_NOTIFY_INFORMATION * info)
	{
		switch (info->Action)
		{
		case FILE_ACTION_ADDED:
			callback({ info->FileName, info->FileNameLength / sizeof(wchar_t) }, FileAction::Added);
			break;

		case FILE_ACTION_REMOVED:
			callback({ info->FileName, info->FileNameLength / sizeof(wchar_t) }, FileAction::Removed);
			break;

		case FILE_ACTION_MODIFIED:
			callback({ info->FileName, info->FileNameLength / sizeof(wchar_t) }, FileAction::Modified);
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:
			callback({ info->FileName, info->FileNameLength / sizeof(wchar_t) }, FileAction::RenamedOldName);
			break;

		case FILE_ACTION_RENAMED_NEW_NAME:
			callback({ info->FileName, info->FileNameLength / sizeof(wchar_t) }, FileAction::RenamedNewName);
			break;

		default:
			callback({ info->FileName, info->FileNameLength / sizeof(wchar_t) }, FileAction::None);
			break;
		}

		return info->NextEntryOffset;
	}
};


class DirectoryWatcherThreadWork
{
	FileActionCallback m_callback;
	DirectoryChanges m_watcher;

	std::shared_ptr<DirectoryWatcherThreadPool> m_pool;
	std::exception_ptr m_exception;
	PTP_WORK m_work;

public:
	DirectoryWatcherThreadWork(FileActionCallback callback, const wchar_t * path) : m_callback(callback), m_watcher(path)
	{
		m_pool = GetDirectoryWatcherThreadPool();
		m_work = m_pool->Submit(this, [](PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK) { ((DirectoryWatcherThreadWork *) context)->Work(); });
	}

	~DirectoryWatcherThreadWork() noexcept
	{
		m_watcher.Cancel();
		m_pool->WaitFor(m_work);
	}

private:

	void Work()
	{
		try
		{
			m_watcher.Watch(m_callback);
		}
		catch (...)
		{
			m_exception = std::current_exception();
		}
	}
};


DirectoryWatcher::DirectoryWatcher(FileActionCallback callback, const wchar_t * path)
{
	m_work = std::make_unique<DirectoryWatcherThreadWork>(callback, path);
}

DirectoryWatcher::DirectoryWatcher()
{}

DirectoryWatcher::~DirectoryWatcher() noexcept
{}

DirectoryWatcher & DirectoryWatcher::operator=(DirectoryWatcher && other) noexcept
{
	m_work = std::move(other.m_work);
	return *this;
}
