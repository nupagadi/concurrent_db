#pragma once

#define HASH_TABLE_START_SIZE 1048576
#define HASH_TABLE_START_SIZE 64

#include <deque>
#include <string>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>

class DB_MUTEX
{
	const size_t HC;
	std::deque<std::mutex> MUTEXES;

public:
	DB_MUTEX(const size_t threads_num) : HC(threads_num), MUTEXES(HC) {}

	std::mutex& get(size_t hash){ return MUTEXES[hash%HC]; }
};


template<class key_t, class mapped_t>
class HashTable
{
public:
	typedef std::pair<key_t, mapped_t> elem_t;

	HashTable(const size_t threads_num, const size_t table_size = HASH_TABLE_START_SIZE) : mMutex(threads_num), mMap(table_size) {}

	mapped_t& operator[](const key_t& key);
	mapped_t& operator[](key_t&& key);

private:
	std::mutex& get_mutex(const key_t& key) { return mMutex.get(std::hash<key_t>()(key)); }

	std::list<elem_t>& bucket(const key_t& key) { return mMap[std::hash<key_t>()(key) % mMap.size()]; }

	std::deque<std::list<elem_t>> mMap;

	DB_MUTEX mMutex;

};

//template<class key_t, class mapped_t>
//DB_MUTEX HashTable<key_t, mapped_t>::MUTEX;


template<class key_t, class mapped_t>
mapped_t& HashTable<key_t, mapped_t>::operator[](const key_t& key)
{
	auto lock_guard = std::lock_guard<std::mutex>(get_mutex(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second;
	else
		return b_list.insert(it, { key, mapped_t() })->second;
}

template<class key_t, class mapped_t>
mapped_t& HashTable<key_t, mapped_t>::operator[](key_t&& key)
{
	auto lock_guard = std::lock_guard<std::mutex>(get_mutex(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second;
	else
		return b_list.insert(it, { std::forward<key_t>(key), mapped_t() })->second;
}

