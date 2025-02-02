#include "pch.h"
#include <command.h>

void PrintTo(const CommandLine & bar, std::ostream * os)
{
	*os << '[';
	PrintTo(bar.File(), *os);
	*os << " ";
	PrintTo(EscapedParameters(bar), *os);
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
{
	CommandLine Clone(CommandLine & other)
	{
		std::wstring command(other.File());

		command.push_back(L' ');
		command.append(EscapedParameters(other));

		return command.c_str();
	}
};

TEST_P(EscapedParametersTest, Case)
{
	CommandLine commandLine1(GetParam());
	CommandLine commandLine2 = Clone(commandLine1);
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


TEST(CommandLineTest, ExecuteSuccess)
{
	CommandLine commandLine(L"cmd /c echo hello");
	EXPECT_EQ(commandLine.Execute(), 0);
}

TEST(CommandLineTest, ExecuteError)
{
	CommandLine commandLine(L"cmd /c exit 1");
	EXPECT_EQ(commandLine.Execute(), 1);
}
