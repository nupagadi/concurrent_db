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

	HashTable(const size_t threads_num, const size_t table_size = HASH_TABLE_START_SIZE) : mMutex(threads_num), mMap(table_size) {}

	mapped_t& operator[](const key_t& key);
	mapped_t& operator[](key_t&& key);

	//REMOVE

	// don't share iterators among several threads
	template<class elem_t>
	class Iterator : public std::iterator<std::bidirectional_iterator_tag, elem_t>
	{
	public:
		typedef typename std::list<elem_t>::iterator list_iterator_t;
		typedef typename std::deque<std::list<elem_t>>::iterator deque_iterator_t;

		//Iterator(deque_iterator_t deque_begin, deque_iterator_t deque_iterator, deque_iterator_t deque_end, list_iterator_t list_iterator, list_iterator_t list_end)
		//	: mDequeBegin(deque_begin), mDequeIterator(deque_iterator), mDequeEnd(deque_end), 
		//	  mListIterator(list_iterator), mListEnd(list_end)
		//{	do_increment(); }

		Iterator(std::deque<std::list<elem_t>>& map, size_t index, list_iterator_t list_iterator, DB_MUTEX& mx) : mMap(map), mIndex(index), mListIterator(list_iterator), mMutex(mx)
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

		bool operator==(const Iterator& rh) {
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



		size_t mIndex = 0;
		std::deque<std::list<elem_t>>& mMap;

		list_iterator_t mListIterator;

		DB_MUTEX& mMutex;
	};

	Iterator<elem_t> Begin()	{
		return Iterator<elem_t>(mMap, 0, mMap[0].begin(), mMutex);
	}
	Iterator<elem_t> End()	{
		return Iterator<elem_t>(mMap, mMap.size(), mMap[mMap.size() - 1].end(), mMutex);
	}

private:
	std::mutex& get_elem_mx(const key_t& key) { return mMutex.GetElemRW(std::hash<key_t>()(key)); }
	std::mutex& get_list_mx(const key_t& key) { return mMutex.GetListAlt(std::hash<key_t>()(key)); }

	std::list<elem_t>& bucket(const key_t& key) { return mMap[std::hash<key_t>()(key) % mMap.size()]; }

	DB_MUTEX mMutex;

	std::atomic<size_t> mSize;
	std::deque<std::list<elem_t>> mMap;
};

//template<class key_t, class mapped_t>
//DB_MUTEX HashTable<key_t, mapped_t>::MUTEX;


template<class key_t, class mapped_t>
mapped_t& HashTable<key_t, mapped_t>::operator[](const key_t& key)
{
	auto lock_guard = std::lock_guard<std::mutex>(get_elem_mx(key));
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

