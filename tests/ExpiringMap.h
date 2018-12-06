#pragma once

#include <chrono>
#include <unordered_map>
#include <map>
#include <type_traits>
#include <mutex>
#include <optional>

namespace ExpiringMapHelper
{
	template<typename T, typename U, typename = std::size_t>
	struct Helper : std::false_type {};

	template<typename T, typename U>
	struct Helper<T, U, decltype(std::declval<std::add_const_t<T>>()(std::declval<U>()))> : std::true_type {};

	template<typename T, typename U>
	constexpr bool is_hash_equivalent = Helper<T, U>::value;
	
}

template<typename T, typename U, typename HASHER = std::hash<T>>
class ExpiringMap
{
public:
	using key_type = T;
	using data_type = U;
	using clock = std::chrono::steady_clock;
	using time_point = std::chrono::time_point<clock>;
	using duration = clock::duration;

	static_assert(std::is_copy_assignable_v<U> || std::is_move_assignable_v<U>);
	static_assert(std::is_copy_constructible_v<U> || std::is_move_constructible_v<U>);
	static_assert(ExpiringMapHelper::is_hash_equivalent<HASHER, T>);

	ExpiringMap(duration data_live_time) : data_live_time{ data_live_time } {};
	ExpiringMap(const ExpiringMap&) = default;
	ExpiringMap& operator=(const ExpiringMap&) = default;
	ExpiringMap(ExpiringMap&&) = default;
	ExpiringMap& operator=(ExpiringMap&&) = default;
	~ExpiringMap() {};

	template<typename R>
	void insert(key_type key, R && data);
	template<typename ... Args>
	void emplace(key_type key, Args && ... args);

	template<typename R>
	bool assign(key_type key, R && data);
	template<typename ... Args>
	bool assign_emplace(key_type key, Args && ... args);

	std::optional<data_type> get(const key_type & key);
	data_type at(const key_type & key);
	data_type operator[](const key_type & key);

	void erase(const key_type & key);
	void clear();
	void removeExpiredData();

	void setDataLiveTime(duration time_duration) { data_live_time = time_duration; };
	duration getDataLiveTime() const { return data_live_time; };

	bool empty() const { return data_buffer.empty(); };
	std::size_t size() const { return data_buffer.size(); };

protected:
	void removeExpirationTime(const key_type & key);
	void addExpirationTime(const key_type & key, time_point && new_expiration_time);

private:
	duration data_live_time;
	clock time_clock;

	std::unordered_map<key_type, std::pair<data_type, time_point>, HASHER> data_buffer;
	std::multimap<time_point, key_type> time_sorted_data;

	std::mutex mx;
};

template<typename T, typename U, typename HASHER>
template<typename R>
inline void ExpiringMap<T, U, HASHER>::insert(key_type key, R && data)
{
	static_assert(std::is_same_v<std::decay_t<R>, std::decay_t<data_type>>, "Invalid type");

	removeExpiredData();

	std::unique_lock<std::mutex> lock{ mx };

	auto current_time = clock::now();

	removeExpirationTime(key);
	data_buffer.insert_or_assign(std::move(key), std::make_pair(std::forward<R>(data), current_time + data_live_time));
	addExpirationTime(key, current_time + data_live_time);
}

template<typename T, typename U, typename HASHER>
template<typename ...Args>
inline void ExpiringMap<T, U, HASHER>::emplace(key_type key, Args && ...args)
{
	removeExpiredData();

	std::unique_lock<std::mutex> lock{ mx };

	auto current_time = clock::now();

	removeExpirationTime(key);
	data_buffer.insert_or_assign(std::move(key), std::make_pair(data_type{ args ... }, current_time + data_live_time));
	addExpirationTime(key, current_time + data_live_time);
}

template<typename T, typename U, typename HASHER>
template<typename R>
inline bool ExpiringMap<T, U, HASHER>::assign(key_type key, R && data)
{
	static_assert(std::is_same_v<std::decay_t<R>, std::decay_t<data_type>>, "Invalid type");

	removeExpiredData();

	std::unique_lock<std::mutex> lock{ mx };

	if (!data_buffer.count(key))
	{
		return false;
	}

	auto current_time = clock::now();

	removeExpirationTime(key);
	data_buffer.insert_or_assign(std::move(key), std::make_pair(std::forward<R>(data), current_time + data_live_time));
	addExpirationTime(key, current_time + data_live_time);

	return true;
}

template<typename T, typename U, typename HASHER>
template<typename ...Args>
inline bool ExpiringMap<T, U, HASHER>::assign_emplace(key_type key, Args && ...args)
{
	removeExpiredData();

	std::unique_lock<std::mutex> lock{ mx };

	if (!data_buffer.count(key))
	{
		return false;
	}

	auto current_time = clock::now();

	removeExpirationTime(key);
	data_buffer.insert_or_assign(std::move(key), std::make_pair(data_type{ args ... }, current_time + data_live_time));
	addExpirationTime(key, current_time + data_live_time);

	return true;
}

template<typename T, typename U, typename HASHER>
inline std::optional<typename ExpiringMap<T, U, HASHER>::data_type> ExpiringMap<T, U, HASHER>::get(const key_type & key)
{
	removeExpiredData();

	std::optional<typename ExpiringMap<T, U>::data_type> data{};

	std::unique_lock<std::mutex> lock{ mx };

	if (data_buffer.count(key))
	{
		auto current_time = clock::now();

		removeExpirationTime(key);
		data = data_buffer.at(key).first;
		data_buffer.at(key).second = current_time + data_live_time;
		addExpirationTime(key, current_time + data_live_time);
	}

	return data;
}

template<typename T, typename U, typename HASHER>
inline typename ExpiringMap<T, U, HASHER>::data_type ExpiringMap<T, U, HASHER>::at(const key_type & key)
{
	auto data = get(key);

	if (data.has_value())
	{
		return data.value();
	}

	throw std::out_of_range{ "ExpiringMap: key doesn't exist" };
}

template<typename T, typename U, typename HASHER>
inline typename ExpiringMap<T, U, HASHER>::data_type ExpiringMap<T, U, HASHER>::operator[](const key_type & key)
{
	return at(key);
}

template<typename T, typename U, typename HASHER>
inline void ExpiringMap<T, U, HASHER>::erase(const key_type & key)
{
	removeExpiredData();

	std::unique_lock<std::mutex> lock{ mx };

	if (data_buffer.count(key))
	{
		removeExpirationTime(key);
		data_buffer.erase(key);
	}

}

template<typename T, typename U, typename HASHER>
inline void ExpiringMap<T, U, HASHER>::clear()
{
	std::unique_lock<std::mutex> lock{ mx };

	data_buffer.clear();
}

template<typename T, typename U, typename HASHER>
inline void ExpiringMap<T, U, HASHER>::removeExpiredData()
{
	std::unique_lock<std::mutex> lock{ mx };

	auto current_time = clock::now();

	for (auto it = time_sorted_data.begin(); it != time_sorted_data.end();)
	{
		if (it->first < current_time)
		{
			data_buffer.erase(it->second);
			time_sorted_data.erase(it++);
		}
		else
		{
			break;
		}
	}
}

template<typename T, typename U, typename HASHER>
inline void ExpiringMap<T, U, HASHER>::removeExpirationTime(const key_type & key)
{
	auto data = data_buffer.find(key);

	if (data == data_buffer.end())
		return;

	auto current_expiration_time = data->second.second;

	auto range = time_sorted_data.equal_range(current_expiration_time);

	for (auto it = range.first; it != range.second; ++it)
	{
		if (it->second == key)
		{
			time_sorted_data.erase(it);

			break;
		}
	}
}

template<typename T, typename U, typename HASHER>
inline void ExpiringMap<T, U, HASHER>::addExpirationTime(const key_type & key, time_point && new_expiration_time)
{
	time_sorted_data.insert(std::make_pair(new_expiration_time, key));
}
