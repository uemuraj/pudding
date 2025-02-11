#include "pch.h"
#include <command.h>
#include <future>

void PrintTo(const CommandLine & bar, std::ostream * os)
{
	*os << '[';
	PrintTo(bar.ToString(), *os);
	*os << ']';
}

TEST(CommandLineTest, Null)
{
	CommandLine commandLine(nullptr);
	EXPECT_STREQ(commandLine.File(), L"");
	EXPECT_TRUE(commandLine.Parameters().empty());
}

TEST(CommandLineTest, Empty)
{
	CommandLine commandLine(L"");
	EXPECT_STREQ(commandLine.File(), L"");
	EXPECT_TRUE(commandLine.Parameters().empty());
}


//
// * https://learn.microsoft.com/ja-jp/windows/win32/api/shellapi/nf-shellapi-commandlinetoargvw
// * https://learn.microsoft.com/ja-jp/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170
//

struct EscapedParametersTest : testing::TestWithParam<const wchar_t *>
{};

TEST_P(EscapedParametersTest, Case)
{
	CommandLine commandLine1(GetParam());
	CommandLine commandLine2(commandLine1.ToString().c_str());
	EXPECT_EQ(commandLine1, commandLine2);
}

INSTANTIATE_TEST_CASE_P(Cases, EscapedParametersTest, testing::Values(
	LR"(file "a b c" d e)",
	LR"(file "ab\"c" "\\" d)",
	LR"(file a\\\b d"e f"g h)",
	LR"(file a\\\"b c d)",
	LR"(file a\\\\"b c" d e)",
	LR"(file a"b"" c d)"
));

TEST(CommandLineTest, SearchExecutableSuccess)
{
	EXPECT_EQ(SearchExecutable(L"C:\\Windows;C:\\Windows\\System32", L"cmd.exe"), L"C:\\Windows\\System32\\cmd.exe");
	EXPECT_EQ(SearchExecutable(L"C:\\Windows;C:\\Windows\\System32", L"cmd", L".exe"), L"C:\\Windows\\System32\\cmd.exe");
	EXPECT_EQ(SearchExecutable(L"C:\\Windows;C:\\Windows\\System32", L"cmd.exe", L".exe"), L"C:\\Windows\\System32\\cmd.exe");
}

TEST(CommandLineTest, SearchExecutableError)
{
	EXPECT_EQ(SearchExecutable(L"C:\\Windows;C:\\Windows\\System32", L"cmd", L".com"), L"");
}

TEST(CommandLineTest, ExecuteSuccess)
{
	std::promise<DWORD> promise;

	ExecuteCallback callback = [&promise](const CommandLine &, DWORD exitCode, std::exception_ptr)
	{
		promise.set_value(exitCode);
	};

	ExecuteCommand(callback, CommandLine(L"cmd.exe /c echo hello"), nullptr, SW_HIDE);

	EXPECT_EQ(promise.get_future().get(), 0);
}

TEST(CommandLineTest, ExecuteError)
{
	std::promise<DWORD> promise;

	ExecuteCallback callback = [&promise](const CommandLine &, DWORD exitCode, std::exception_ptr)
	{
		promise.set_value(exitCode);
	};

	ExecuteCommand(callback, CommandLine(L"cmd.exe /c exit 1"), nullptr, SW_HIDE);

	EXPECT_EQ(promise.get_future().get(), 1);
}

TEST(CommandLineTest, ExecuteEnvironment)
{
	std::promise<DWORD> promise;

	ExecuteCallback callback = [&promise](const CommandLine &, DWORD exitCode, std::exception_ptr)
	{
		promise.set_value(exitCode);
	};

	ExecuteCommand(callback, CommandLine(L"cmd.exe /c set > enviroment.txt"), nullptr, SW_HIDE);

	EXPECT_EQ(promise.get_future().get(), 0);
}
