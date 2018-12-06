#pragma once

#include <thread>

#include "ExpiringMap.h"

TEST(ExpiringMap, insert)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 10h };

	expiring_map.insert(10, 45);

	auto x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 45);

	expiring_map.insert(20, 5);
	expiring_map.insert(5, 20);

	x = expiring_map.get(5);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 20);

	x = expiring_map.get(20);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 5);

	expiring_map.insert(10, 0);

	x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 0);
}

TEST(ExpiringMap, emplace)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, std::pair<int, int>> expiring_map{ 10h };

	expiring_map.emplace(10, 45, 30);

	auto x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), std::make_pair(45, 30));
}

TEST(ExpiringMap, assign)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 10h };

	expiring_map.insert(10, 45);

	auto x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 45);

	auto success = expiring_map.assign(10, 10);

	x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(success, true);
	EXPECT_EQ(x.value(), 10);

	success = expiring_map.assign(20, 5);

	EXPECT_EQ(success, false);
}

TEST(ExpiringMap, assign_emplace)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, std::pair<int, int>> expiring_map{ 10h };

	expiring_map.insert(10, std::make_pair(1, 1));

	auto success = expiring_map.assign_emplace(10, 45, 30);

	auto x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(success, true);
	EXPECT_EQ(x.value(), std::make_pair(45, 30));

	success = expiring_map.assign_emplace(20, 45, 30);

	EXPECT_EQ(success, false);
}

TEST(ExpiringMap, get)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 10h };

	expiring_map.insert(10, 45);

	auto x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 45);

	x = expiring_map.get(25);

	EXPECT_EQ(x.has_value(), false);
}

TEST(ExpiringMap, at1)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 10h };

	expiring_map.insert(10, 45);

	auto x = expiring_map.at(10);
	EXPECT_EQ(x, 45);

	EXPECT_THROW(expiring_map.at(25), std::out_of_range);

	try {
		x = expiring_map.at(25);
	}
	catch (const std::out_of_range & out)
	{
		EXPECT_EQ(std::strcmp(out.what(), "ExpiringMap: key doesn't exist"), 0);
	}
}

TEST(ExpiringMap, at2)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 10h };

	expiring_map.insert(10, 45);

	auto x = expiring_map[10];
	EXPECT_EQ(x, 45);

	EXPECT_THROW(expiring_map[25], std::out_of_range);

	try {
		x = expiring_map[25];
	}
	catch (const std::out_of_range & out)
	{
		EXPECT_EQ(std::strcmp(out.what(), "ExpiringMap: key doesn't exist"), 0);
	}
}

TEST(ExpiringMap, erase)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 10h };

	expiring_map.insert(10, 45);

	auto x = expiring_map[10];
	EXPECT_EQ(x, 45);

	expiring_map.erase(10);

	EXPECT_THROW(expiring_map[10], std::out_of_range);
}

TEST(ExpiringMap, data_live_time1)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 2ms };

	expiring_map.insert(10, 45);

	auto x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), true);

	std::this_thread::sleep_for(2ms);

	x = expiring_map.get(10);

	EXPECT_EQ(x.has_value(), false);
}

TEST(ExpiringMap, data_live_time2)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 2ms };

	expiring_map.insert(10, 45);
	expiring_map.emplace(11, 45);
	expiring_map.insert(12, 0);
	expiring_map.assign(12, 45);
	expiring_map.insert(13, 0);
	expiring_map.assign_emplace(13, 45);
	expiring_map.insert(20, 45);
	expiring_map.insert(30, 45);

	auto x = expiring_map.get(10);
	EXPECT_EQ(x.has_value(), true);
	x = expiring_map.get(11);
	EXPECT_EQ(x.has_value(), true);
	x = expiring_map.get(12);
	EXPECT_EQ(x.has_value(), true);
	x = expiring_map.get(13);
	EXPECT_EQ(x.has_value(), true);

	auto old_duration = expiring_map.getDataLiveTime();
	expiring_map.setDataLiveTime(1h);
	x = expiring_map.get(20);
	expiring_map.setDataLiveTime(old_duration);

	EXPECT_EQ(x.has_value(), true);
	x = expiring_map.get(30);
	EXPECT_EQ(x.has_value(), true);

	std::this_thread::sleep_for(2ms);

	x = expiring_map.get(10);
	EXPECT_EQ(x.has_value(), false);
	x = expiring_map.get(11);
	EXPECT_EQ(x.has_value(), false);
	x = expiring_map.get(12);
	EXPECT_EQ(x.has_value(), false);
	x = expiring_map.get(13);
	EXPECT_EQ(x.has_value(), false);
	x = expiring_map.get(20);
	EXPECT_EQ(x.has_value(), true);
	x = expiring_map.get(30);
	EXPECT_EQ(x.has_value(), false);
}

TEST(ExpiringMap, clear_size_empty)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> expiring_map{ 2h };

	expiring_map.insert(10, 45);
	expiring_map.insert(20, 45);
	expiring_map.insert(30, 45);
	expiring_map.insert(40, 45);
	expiring_map.insert(50, 45);

	EXPECT_EQ(expiring_map.size(), 5);

	expiring_map.clear();

	EXPECT_EQ(expiring_map.size(), 0);
	EXPECT_EQ(expiring_map.empty(), true);
}