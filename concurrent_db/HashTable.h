#pragma once

#define HASH_TABLE_START_SIZE 1048576
#define HASH_TABLE_START_SIZE 64

#include <deque>
#include <string>
#include <list>
#include <algorithm>

template<class key_t, class mapped_t>
class HashTable
{
public:
	typedef std::pair<key_t, mapped_t> elem_t;

	HashTable() : mMap(HASH_TABLE_START_SIZE) {}

	mapped_t& Set(const key_t& key, const mapped_t& value);
	mapped_t& Get(const key_t& key);

	mapped_t& operator[](const key_t& key);
	mapped_t& operator[](key_t&& key);

private:
	std::list<elem_t>& bucket(const key_t& key) { return mMap[std::hash<key_t>()(key) % mMap.size()]; }

	std::deque<std::list<elem_t>> mMap;



};

template<class key_t, class mapped_t>
mapped_t& HashTable<key_t, mapped_t>::operator[](const key_t& key)
{
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
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second;
	else
		return b_list.insert(it, { key, mapped_t() })->second;
}

