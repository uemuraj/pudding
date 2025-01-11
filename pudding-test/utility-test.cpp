#include "pch.h"
#include <utility.h>

TEST(UtilityTest, GetCurrentModuleFileName)
{
	EXPECT_EQ(GetCurrentModuleFileName(), VS_TARGETPATH);
}
