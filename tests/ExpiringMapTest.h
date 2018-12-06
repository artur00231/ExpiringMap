#pragma once

#include <thread>

#include "ExpiringMap.h"

TEST(ExpiringMap, insert)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> db{ 10h };

	db.insert(10, 45);

	auto x = db.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 45);

	db.insert(20, 5);
	db.insert(5, 20);

	x = db.get(5);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 20);

	x = db.get(20);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 5);

	db.insert(10, 0);

	x = db.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 0);
}

TEST(ExpiringMap, emplace)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, std::pair<int, int>> db{ 10h };

	db.emplace(10, 45, 30);

	auto x = db.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), std::make_pair(45, 30));
}

TEST(ExpiringMap, assign)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> db{ 10h };

	db.insert(10, 45);

	auto x = db.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 45);

	auto success = db.assign(10, 10);

	x = db.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(success, true);
	EXPECT_EQ(x.value(), 10);

	success = db.assign(20, 5);

	EXPECT_EQ(success, false);
}

TEST(ExpiringMap, assign_emplace)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, std::pair<int, int>> db{ 10h };

	db.insert(10, std::make_pair(1, 1));

	auto success = db.assign_emplace(10, 45, 30);

	auto x = db.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(success, true);
	EXPECT_EQ(x.value(), std::make_pair(45, 30));

	success = db.assign_emplace(20, 45, 30);

	EXPECT_EQ(success, false);
}

TEST(ExpiringMap, get)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> db{ 10h };

	db.insert(10, 45);

	auto x = db.get(10);

	EXPECT_EQ(x.has_value(), true);
	EXPECT_EQ(x.value(), 45);

	x = db.get(25);

	EXPECT_EQ(x.has_value(), false);
}

TEST(ExpiringMap, data_live_time1)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> db{ 2ms };

	db.insert(10, 45);

	auto x = db.get(10);

	EXPECT_EQ(x.has_value(), true);

	std::this_thread::sleep_for(2ms);

	x = db.get(10);

	EXPECT_EQ(x.has_value(), false);
}

TEST(ExpiringMap, data_live_time2)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> db{ 2ms };

	db.insert(10, 45);
	db.emplace(11, 45);
	db.insert(12, 0);
	db.assign(12, 45);
	db.insert(13, 0);
	db.assign_emplace(13, 45);
	db.insert(20, 45);
	db.insert(30, 45);

	auto x = db.get(10);
	EXPECT_EQ(x.has_value(), true);
	x = db.get(11);
	EXPECT_EQ(x.has_value(), true);
	x = db.get(12);
	EXPECT_EQ(x.has_value(), true);
	x = db.get(13);
	EXPECT_EQ(x.has_value(), true);

	auto old_duration = db.getDataLiveTime();
	db.setDataLiveTime(1h);
	x = db.get(20);
	db.setDataLiveTime(old_duration);

	EXPECT_EQ(x.has_value(), true);
	x = db.get(30);
	EXPECT_EQ(x.has_value(), true);

	std::this_thread::sleep_for(2ms);

	x = db.get(10);
	EXPECT_EQ(x.has_value(), false);
	x = db.get(11);
	EXPECT_EQ(x.has_value(), false);
	x = db.get(12);
	EXPECT_EQ(x.has_value(), false);
	x = db.get(13);
	EXPECT_EQ(x.has_value(), false);
	x = db.get(20);
	EXPECT_EQ(x.has_value(), true);
	x = db.get(30);
	EXPECT_EQ(x.has_value(), false);
}

TEST(ExpiringMap, clear_size_empty)
{
	using namespace std::chrono_literals;

	ExpiringMap<int, int> db{ 2h };

	db.insert(10, 45);
	db.insert(20, 45);
	db.insert(30, 45);
	db.insert(40, 45);
	db.insert(50, 45);

	EXPECT_EQ(db.size(), 5);

	db.clear();

	EXPECT_EQ(db.size(), 0);
	EXPECT_EQ(db.empty(), true);
}