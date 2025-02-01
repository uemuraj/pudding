#pragma once

#include <Windows.h>

class CriticalSection : CRITICAL_SECTION
{
public:
	CriticalSection() noexcept
	{
		::InitializeCriticalSection(this);
	}

	~CriticalSection() noexcept
	{
		::DeleteCriticalSection(this);
	}

	operator CRITICAL_SECTION * () noexcept
	{
		return this;
	}
};

class LockGuard
{
	CRITICAL_SECTION * m_section;

public:
	LockGuard(CRITICAL_SECTION * section) noexcept : m_section(section)
	{
		::EnterCriticalSection(m_section);
	}

	~LockGuard() noexcept
	{
		::LeaveCriticalSection(m_section);
	}
};

class StopEvent
{
	HANDLE m_event;

public:
	StopEvent();
	~StopEvent() noexcept;

	operator HANDLE() const noexcept
	{
		return m_event;
	}
};


class ThreadPool
{
	PTP_POOL m_pool;

public:
	ThreadPool(const ThreadPool &) = delete;
	ThreadPool & operator = (const ThreadPool &) = delete;

	ThreadPool(unsigned long minimum, unsigned long maximum);
	~ThreadPool() noexcept;

	operator PTP_POOL() const noexcept
	{
		return m_pool;
	}
};

class ThreadpoolCleanupGroup
{
	PTP_CLEANUP_GROUP m_group;

public:
	ThreadpoolCleanupGroup(const ThreadpoolCleanupGroup &) = delete;
	ThreadpoolCleanupGroup & operator = (const ThreadpoolCleanupGroup &) = delete;

	ThreadpoolCleanupGroup();
	~ThreadpoolCleanupGroup() noexcept;

	operator PTP_CLEANUP_GROUP() const noexcept
	{
		return m_group;
	}
};

class ThreadpoolEnviroment : public TP_CALLBACK_ENVIRON
{
	ThreadPool m_pool;
	ThreadpoolCleanupGroup m_group;

public:
	ThreadpoolEnviroment(bool runsLong, unsigned long minimum, unsigned long maximum);
	~ThreadpoolEnviroment() noexcept;

	PTP_WORK Submit(void * context, PTP_WORK_CALLBACK fnWork);
	void WaitFor(PTP_WORK work) noexcept;
};
