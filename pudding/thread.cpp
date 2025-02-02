#include "thread.h"
#include <system_error>


StopEvent::StopEvent() : m_event(::CreateEvent(nullptr, true, false, nullptr))
{
	if (!m_event)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "CreateEvent()");
	}
}

StopEvent::~StopEvent() noexcept
{
	::CloseHandle(m_event);
}

Semaphore::Semaphore(long initial, long maximum) : m_semaphore(::CreateSemaphore(nullptr, initial, maximum, nullptr))
{
	if (!m_semaphore)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "CreateSemaphore()");
	}
}

Semaphore::~Semaphore() noexcept
{
	::CloseHandle(m_semaphore);
}


ThreadPool::ThreadPool(unsigned long minimum, unsigned long maximum) : m_pool(::CreateThreadpool(nullptr))
{
	if (!m_pool)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "CreateThreadpool()");
	}

	if (!::SetThreadpoolThreadMinimum(m_pool, minimum))
	{
		auto error = ::GetLastError();

		::CloseThreadpool(m_pool); // !!!

		throw std::system_error(error, std::system_category(), "SetThreadpoolThreadMinimum()");
	}

	::SetThreadpoolThreadMaximum(m_pool, maximum);
}

ThreadPool::~ThreadPool() noexcept
{
	::CloseThreadpool(m_pool);
}


ThreadpoolCleanupGroup::ThreadpoolCleanupGroup() : m_group(::CreateThreadpoolCleanupGroup())
{
	if (!m_group)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "CreateThreadpoolCleanupGroup()");
	}
}

ThreadpoolCleanupGroup::~ThreadpoolCleanupGroup() noexcept
{
	::CloseThreadpoolCleanupGroup(m_group);
}


ThreadpoolEnviroment::ThreadpoolEnviroment(bool runsLong, unsigned long minimum, unsigned long maximum) : m_pool(minimum, maximum)
{
	::InitializeThreadpoolEnvironment(this);

	if (runsLong)
	{
		::SetThreadpoolCallbackRunsLong(this);
	}

	::SetThreadpoolCallbackCleanupGroup(this, m_group, nullptr);
	::SetThreadpoolCallbackPool(this, m_pool);
}

ThreadpoolEnviroment::~ThreadpoolEnviroment() noexcept
{
	::CloseThreadpoolCleanupGroupMembers(m_group, true, nullptr);
	::DestroyThreadpoolEnvironment(this);
}

PTP_WORK ThreadpoolEnviroment::SubmitWork(void * context, PTP_WORK_CALLBACK fnWork)
{
	auto work = ::CreateThreadpoolWork(fnWork, context, this);

	if (!work)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "CreateThreadpoolWork()");
	}

	::SubmitThreadpoolWork(work);

	return work;
}

void ThreadpoolEnviroment::WaitForWork(PTP_WORK work) noexcept
{
	::WaitForThreadpoolWorkCallbacks(work, true);

	::CloseThreadpoolWork(work);
}
