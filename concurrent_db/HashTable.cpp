#include <algorithm>

#include "HashTable.h"


HashTable::mapped_t& HashTable::Get(const key_t& key)
{
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second;
	else
		return b_list.insert(it, { key, mapped_t() })->second;
}

HashTable::mapped_t& HashTable::Set(const key_t& key, const mapped_t& value)
{
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second = value;
	else
		return b_list.insert(it, { key, value })->second;
}
