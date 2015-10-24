#pragma once

#define HASH_TABLE_START_SIZE 1048576
#define HASH_TABLE_START_SIZE 4

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
	std::deque<std::mutex> mElemRW;
	std::deque<std::mutex> mListAlt;

public:
	DB_MUTEX(const size_t threads_num) : mThreadsNum(threads_num), mElemRW(mThreadsNum), mListAlt(mThreadsNum) {}

	std::mutex& GetElemRW(size_t hash){ return mElemRW[hash%mThreadsNum]; }
	std::mutex& GetListAlt(size_t index){ return mListAlt[index%mThreadsNum]; }
};


template<class key_t, class mapped_t>
class HashTable
{
public:
	typedef std::pair<key_t, mapped_t> elem_t;

	template<class elem_t>	class Entry;
	template<class elem_t>	class Iterator;

	HashTable(const size_t threads_num, const size_t table_size = HASH_TABLE_START_SIZE) : mMutex(threads_num), mMap(table_size) {}

	mapped_t& operator[](const key_t& key);
	mapped_t& operator[](key_t&& key);

	//REMOVE
	
	Iterator<Entry<elem_t>> Begin()	{
		return Iterator<Entry<elem_t>>(mMap, 0, mMap[0].begin(), mMutex);
	}
	Iterator<Entry<elem_t>> End()	{
		return Iterator<Entry<elem_t>>(mMap, mMap.size(), mMap[mMap.size() - 1].end(), mMutex);
	}

private:
	std::mutex& get_elem_mx(const key_t& key) { return mMutex.GetElemRW(std::hash<key_t>()(key)); }
	std::mutex& get_list_mx(const key_t& key) { return mMutex.GetListAlt(std::hash<key_t>()(key)); }

	std::list<Entry<elem_t>>& bucket(const key_t& key) { return mMap[std::hash<key_t>()(key) % mMap.size()]; }

	DB_MUTEX mMutex;

	std::atomic<size_t> mSize;
	std::deque<std::list<Entry<elem_t>>> mMap;

public:

	template<class elem_t>	struct Entry : public elem_t
	{
		//Entry(const Entry& rh){}
		//Entry(const elem_t& rh){}
		//Entry(elem_t&& rh){}
		Entry(const key_t& key, const mapped_t& value)/* : mPair( key, value )*/
		{	
			mPair.first = key; 
			mPair.second = value; 
		}
		Entry(key_t&& key, mapped_t&& value) /*: mPair(std::forward<key_t>(key), std::forward<mapped_t>(value))*/
		{ 
			mPair.first = key; 
			mPair.second = value; 
		}
		//Entry& operator=(const Entry& rh)
		//{
		//	return *this;
		//}
		//Entry& operator=(const elem_t& rh)
		//{
		//	return *this;
		//}
		//Entry& operator=(elem_t&& rh)
		//{
		//	return *this;
		//}

		elem_t mPair;
	};
	
	// don't share iterators among several threads
	template<class Entry>
	class Iterator : public std::iterator<std::bidirectional_iterator_tag, Entry>
	{
	public:
		typedef typename std::list<Entry>::iterator list_iterator_t;

		Iterator(std::deque<std::list<Entry>>& map, size_t index, list_iterator_t list_iterator, DB_MUTEX& mx) : mMap(map), mIndex(index), mListIterator(list_iterator), mMutex(mx)
		{
			mMutex.GetElemRW(mIndex).lock();
			bool is_empty = mIndex != mMap.size() && mMap[mIndex].empty();
			mMutex.GetElemRW(mIndex).unlock();
			if (is_empty)	do_increment();
		}

		// off-the-end HashTable::Iterator should not be incremented
		Iterator operator++() {
			do_increment();
			return *this;
		}
		Iterator operator++(int) {
			auto prev = *this;
			do_increment();
			return prev;
		}

		// begin HashTable::Iterator should not be decremented
		Iterator operator--() {
			do_decrement();
			return *this;
		}
		Iterator operator--(int) {
			auto next = *this;
			do_decrement();
			return next;
		}

		bool operator==(const Iterator& rh) const {
			if (mIndex == this->mMap.size())
				return mIndex == rh.mIndex;
			return mIndex == rh.mIndex && mListIterator == rh.mListIterator;				
		}
		bool operator!=(const Iterator& rh) { return !operator==(rh); }

		elem_t& operator*(){ return *mListIterator; }

	private:
		void do_increment()
		{

			// CONSIDER GUARDS!!!!!!!!!!
			// CONSIDER GUARDS!!!!!!!!!!
			// CONSIDER GUARDS!!!!!!!!!!
			// in case of incrementing end or decrementing begin

			mMutex.GetElemRW(mIndex).lock();
			auto list_end = mMap[mIndex].end();

			if (mListIterator != list_end)
				++mListIterator;

			if (mListIterator == list_end)
			{
				do	{
					// order matters due to deadlock possibility
					mMutex.GetElemRW(mIndex).unlock();

					if (++mIndex == mMap.size())	return;

					mMutex.GetElemRW(mIndex).lock();
				}	while (mMap[mIndex].empty());
				
				mListIterator = mMap[mIndex].begin();

			}
			// unlock last index mutex, mb original one
			mMutex.GetElemRW(mIndex).unlock();
		}

		void do_decrement()
		{

			// CONSIDER GUARDS!!!!!!!!!!
			// CONSIDER GUARDS!!!!!!!!!!
			// CONSIDER GUARDS!!!!!!!!!!

			mMutex.GetElemRW(mIndex).lock();

			if (mIndex == mMap.size())
				seek_prev();

			//if (mIndex != mMap.size())
			else
			{
				auto list_begin = mMap[mIndex].begin();
				// assume mListIterator is valid
				if (mListIterator != list_begin)
					--mListIterator;
				else	seek_prev();
			}
			// unlock last index mutex, mb original one
			mMutex.GetElemRW(mIndex).unlock();
		}

		// seek for the very previous valid iterator
		// unlocks input, locks output
		void seek_prev()
		{
			
			do {
				// order matters due to deadlock possibility
				mMutex.GetElemRW(mIndex).unlock();
				mMutex.GetElemRW(--mIndex).lock();
			} while (mIndex > 0 && mMap[mIndex].empty());

			if (mIndex >= 0 && !mMap[mIndex].empty())
			{
				mListIterator = mMap[mIndex].end();
				--mListIterator;
			}
		}

		list_iterator_t mListIterator;
		size_t mIndex = 0;
		std::deque<std::list<Entry>>& mMap;

		DB_MUTEX& mMutex;
	};

};

template<class key_t, class mapped_t>
mapped_t& HashTable<key_t, mapped_t>::operator[](const key_t& key)
{
	auto lock_guard = std::lock_guard<std::mutex>(get_elem_mx(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const Entry<elem_t>& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second;
	else
	{
		mSize.fetch_add(1);
		return b_list.emplace(it, key, mapped_t())->second;
	}
}

template<class key_t, class mapped_t>
mapped_t& HashTable<key_t, mapped_t>::operator[](key_t&& key)
{
	auto lock_guard = std::lock_guard<std::mutex>(get_elem_mx(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const Entry<elem_t>& el){ return el.first == key; });
	if (it != b_list.end())
		return it->second;
	else
	{
		mSize.fetch_add(1);
		auto inserted = b_list.emplace(it, std::forward<key_t>(key), mapped_t());
		return inserted->second;
	}
}

