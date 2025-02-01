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


// TODO: スレッドの最大数を超えないよう Submit 可能な数を制限する

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

PTP_WORK ThreadpoolEnviroment::Submit(void * context, PTP_WORK_CALLBACK fnWork)
{
	auto work = ::CreateThreadpoolWork(fnWork, context, this);

	if (!work)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "CreateThreadpoolWork()");
	}

	::SubmitThreadpoolWork(work);

	return work;
}

void ThreadpoolEnviroment::WaitFor(PTP_WORK work) noexcept
{
	::WaitForThreadpoolWorkCallbacks(work, true);

	::CloseThreadpoolWork(work);
}
