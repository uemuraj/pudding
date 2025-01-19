#include "pch.h"
#include <profile.h>

TEST(ProfileTest, LoadProfile)
{
	auto profile = LoadProfile();
	auto & section1 = profile->at(L"section1");
	auto & section2 = profile->at(L"section2");

	EXPECT_EQ(profile->size(), 2);
	EXPECT_EQ(section1.size(), 2);
	EXPECT_EQ(section2.size(), 2);

	EXPECT_EQ(section1.at(L"key1"), L"value1");
	EXPECT_EQ(section1.at(L"key2"), L"value2");
	EXPECT_EQ(section2.at(L"key3"), L"value3");
	EXPECT_EQ(section2.at(L"key4"), L"value4");
}

TEST(ProfileTest, LoadProfileFail)
{
	auto profile = LoadProfile(L"");
	EXPECT_EQ(profile->size(), 0);
}

TEST(ProfileTest, LoadProfileNoSection)
{
	auto profile = LoadProfile();
	auto & section3 = profile->operator[](L"section3");
	auto & section4 = profile->operator[](L"section4");

	EXPECT_EQ(profile->size(), 4);
	EXPECT_EQ(section3.size(), 0);
	EXPECT_EQ(section4.size(), 0);
}

TEST(ProfileTest, LoadProfileNoKey)
{
	auto profile = LoadProfile();
	auto & section1 = profile->at(L"section1");
	auto & section2 = profile->at(L"section2");

	EXPECT_EQ(profile->size(), 2);
	EXPECT_EQ(section1.size(), 2);
	EXPECT_EQ(section2.size(), 2);

	EXPECT_EQ(section1[L"key3"], L"");
	EXPECT_EQ(section1[L"key4"], L"");
	EXPECT_EQ(section2[L"key1"], L"");
	EXPECT_EQ(section2[L"key2"], L"");

	EXPECT_EQ(section1.size(), 4);
	EXPECT_EQ(section2.size(), 4);
}
