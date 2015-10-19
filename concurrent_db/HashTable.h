#pragma once

#define HASH_TABLE_START_SIZE 1048576
#define HASH_TABLE_START_SIZE 64

#include <deque>
#include <string>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

class DB_MUTEX
{
	const size_t mThreadsNum;
	std::deque<std::mutex> mElemMX;
	std::deque<std::mutex> mListMX;

public:
	DB_MUTEX(const size_t threads_num) : mThreadsNum(threads_num), mElemMX(mThreadsNum), mListMX(mThreadsNum) {}

	std::mutex& GetElemMX(size_t hash){ return mElemMX[hash%mThreadsNum]; }
	std::mutex& GetListMX(size_t index){ return mListMX[index%mThreadsNum]; }
};


template<class key_t, class mapped_t>
class HashTable
{
public:
	typedef std::pair<key_t, mapped_t> elem_t;

	HashTable(const size_t threads_num, const size_t table_size = HASH_TABLE_START_SIZE) : mMutex(threads_num), mMap(table_size) {}

	mapped_t& operator[](const key_t& key);
	mapped_t& operator[](key_t&& key);

	//REMOVE

	template<class key_t, class mapped_t>
	class Iterator : public std::iterator<std::bidirectional_iterator_tag, mapped_t>
	{
	public:
		typedef typename std::list<elem_t>::iterator list_iterator_t;
		typedef typename std::deque<std::list<elem_t>>::iterator deque_iterator_t;

		Iterator(deque_iterator_t deque_iterator, deque_iterator_t deque_end, list_iterator_t list_iterator, list_iterator_t list_end)
			: mDequeIterator(deque_iterator), mDequeEnd(deque_end), mListIterator(list_iterator), mListEnd(list_end)
			{}

		// off-the-end HashTable::Iterator should not be incremented
		Iterator& operator++() {
			do_increment();
			return *this;
		}
		Iterator& operator++(int) {
			do_increment();
			return *this;
		}

		// begin HashTable::Iterator should not be decremented
		Iterator& operator--() {
			--mListIterator;
			if (mListIterator == mListEnd)
			{
				++mDequeIterator;
				mListIterator = mDequeIterator->begin();
				mListEnd = mDequeIterator->end();
			}
			return *this;
		}

	private:
		void do_increment()
		{
			// BLOCK LIST MUTEX
			++mListIterator;
			if (mListIterator == mListEnd)
			{
				++mDequeIterator;
				mListIterator = mDequeIterator->begin();
				mListEnd = mDequeIterator->end();
			}
		}

		deque_iterator_t mDequeIterator;
		deque_iterator_t mDequeEnd;
		list_iterator_t mListIterator;
		list_iterator_t mListEnd;
	};

	Iterator<key_t, mapped_t> Begin()	{
		return Iterator<key_t, mapped_t>(mMap.begin(), mMap.end(), mMap.begin()->begin(), mMap.end()->end());
	}
	Iterator<key_t, mapped_t> End()	{
		return Iterator<key_t, mapped_t>(mMap.begin(), mMap.end(), mMap.begin()->begin(), mMap.end()->end());
	}

private:
	std::mutex& get_elem_mx(const key_t& key) { return mMutex.GetElemMX(std::hash<key_t>()(key)); }
	std::mutex& get_list_mx(const key_t& key) { return mMutex.GetListMX(std::hash<key_t>()(key)); }

	std::list<elem_t>& bucket(const key_t& key) { return mMap[std::hash<key_t>()(key) % mMap.size()]; }

	std::deque<std::list<elem_t>> mMap;

	DB_MUTEX mMutex;

	std::atomic<size_t> mSize;
};

//template<class key_t, class mapped_t>
//DB_MUTEX HashTable<key_t, mapped_t>::MUTEX;


template<class key_t, class mapped_t>
mapped_t& HashTable<key_t, mapped_t>::operator[](const key_t& key)
{
	auto list_lock = std::lock_guard<std::mutex>(get_list_mx(key));
	auto elem_lock = std::lock_guard<std::mutex>(get_elem_mx(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second;
	else
	{
		// BLOCK LIST MUTEX
		mSize.fetch_add(1);
		return b_list.insert(it, { key, mapped_t() })->second;
	}
}

template<class key_t, class mapped_t>
mapped_t& HashTable<key_t, mapped_t>::operator[](key_t&& key)
{
	auto list_lock = std::lock_guard<std::mutex>(get_list_mx(key));
	auto lock_guard = std::lock_guard<std::mutex>(get_elem_mx(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second;
	else
	{
		mSize.fetch_add(1);
		return b_list.insert(it, { std::forward<key_t>(key), mapped_t() })->second;
	}
}

