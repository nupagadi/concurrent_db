#pragma once

#define HASH_TABLE_START_SIZE 1048576
#define HASH_TABLE_START_SIZE 64

#include <deque>
#include <string>
#include <list>

//template
class HashTable
{
public:
	typedef std::string key_t;
	typedef std::string mapped_t;
	typedef std::pair<key_t, mapped_t> elem_t;

	HashTable() : mMap(HASH_TABLE_START_SIZE) {}

	mapped_t& Set(const key_t& key, const mapped_t& value);
	mapped_t& Get(const key_t& key);

	mapped_t& operator[](const key_t& key);

private:
	std::list<elem_t>& bucket(const key_t& key) { return mMap[std::hash<key_t>()(key) % mMap.size()]; }

	std::deque<std::list<elem_t>> mMap;



};