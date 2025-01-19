//
// pch.cpp
//

#include "pch.h"

#include <stdio.h>
#include <stdlib.h>
#include <comdef.h>

#include <locale>
#include <system_error>

struct WindowsEnvironment : public ::testing::Environment
{
	void SetUp() override
	{
		std::locale::global(std::locale(""));

		if (auto hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "CoInitializeEx()");
		}
	}

	void TearDown() override
	{
		::CoUninitialize();
	}
};

int main(int argc, char ** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new WindowsEnvironment);
	return RUN_ALL_TESTS();
}


void PrintTo(const std::wstring & str, std::ostream & os)
{
	std::string buff;

	for (auto ch : str)
	{
		buff.resize(MB_CUR_MAX);

#pragma warning(suppress: 4996)
		if (int size = wctomb(buff.data(), ch); size > 0)
		{
			buff.resize(size);

			os << buff;
		}
	}
}
