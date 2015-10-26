#pragma once

#define HASH_TABLE_START_SIZE 1048576*2
//#define HASH_TABLE_START_SIZE 64

//#include <deque>
#include <vector>
#include <string>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

#include <windows.h>

class CRITICAL_SECTION_CLASS
{
	CRITICAL_SECTION mCS;
public:
	CRITICAL_SECTION_CLASS() { InitializeCriticalSection(&mCS); }
	~CRITICAL_SECTION_CLASS(){ DeleteCriticalSection(&mCS); }
	void lock(){ EnterCriticalSection(&mCS); }
	void unlock(){ LeaveCriticalSection(&mCS); }
};

template<class mutex_t>
class DB_MUTEX
{
	const size_t mThreadsNum;

	std::vector<mutex_t> mElemRW;

public:
	DB_MUTEX(size_t threads_num) : mThreadsNum(threads_num), mElemRW(mThreadsNum) {}

	mutex_t& GetElemRW(size_t hash){ return mElemRW[hash%mThreadsNum]; }

	void lock(size_t hash){ mElemRW[hash%mThreadsNum].lock(); }
	void unlock(size_t hash){ mElemRW[hash%mThreadsNum].unlock(); }
};

template<class key_t, class mapped_t, class mutex_t>
class HashTable
{
public:
	// const key_t
	typedef std::pair<const key_t, mapped_t> elem_t;

	template<class elem_t>		class Iterator;
	template<class mapped_t>	class Entry;

	HashTable(const size_t threads_num, const size_t table_size = HASH_TABLE_START_SIZE) : mMutex(threads_num), mArray(table_size) {}

	//Entry<elem_t> operator[](const key_t& key);

	// template function for perfect forwarding
	template<class key_t2>
	Entry<elem_t> operator[](key_t2&& key);

	template<class key_t2>
	mapped_t load(key_t2&& key);

	template<class key_t2, class mapped_t2>
	void store(key_t2&& key, mapped_t2&& value);

	// number of elements
	size_t ElemNum() { return mSize.load(); }

	void Erase(const key_t& key);
	// end iterator deletion does nothing
	void Erase(const Iterator<elem_t>& it) {
		if (it.Index() == mArray.size())		return;
		mArray[it.Index()].erase(it.ListIterator());
	}

	template<class elem_t>
	class Entry
	{
	public:
		Entry(elem_t& entry, DB_MUTEX<mutex_t>& mutex) : mEntry(entry), mMutex(mutex)
		{}
		Entry& operator=(const mapped_t& rh)
		{
			auto lock_guard = std::lock_guard<mutex_t>(mMutex.GetElemRW(std::hash<key_t>()(mEntry.first)));
			mEntry.second = rh;
			return *this;
		}
		Entry& operator=(mapped_t&& rh)
		{
			auto lock_guard = std::lock_guard<mutex_t>(mMutex.GetElemRW(std::hash<key_t>()(mEntry.first)));
			mEntry.second = std::move(rh);
			return *this;
		}

		operator mapped_t() { 
			auto lock_guard = std::lock_guard<mutex_t>(mMutex.GetElemRW(std::hash<key_t>()(mEntry.first)));
			return mEntry.second; 
		}

	private:
		elem_t& mEntry;
		DB_MUTEX<mutex_t>& mMutex;
	};

private:
	template<class key_t2>
	mutex_t& get_elem_mx(key_t2&& key) { return mMutex.GetElemRW(mHash(key)); }
	void lock(const key_t& key) { mMutex.lock(std::hash<key_t>()(key)); }
	void unlock(const key_t& key) { mMutex.unlock(std::hash<key_t>()(key)); }

	template<class key_t2>
	std::list<elem_t>& bucket(const key_t2& key) { return mArray[mHash(key) % mArray.size()]; }
	std::pair<std::list<elem_t>, bool> find(const key_t& key){
		auto& b_list = bucket(key);
		auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
		return std::make_pair(it, it != b_list.end());
	}

	std::hash<key_t> mHash;

	DB_MUTEX<mutex_t> mMutex;

	std::atomic<size_t> mSize;
	std::vector<std::list<elem_t>> mArray;

public:
	// don't share iterators among several threads
	template<class elem_t>
	class Iterator : public std::iterator<std::bidirectional_iterator_tag, elem_t>
	{
	public:
		typedef typename std::list<elem_t>::iterator list_iterator_t;

		Iterator(std::vector<std::list<elem_t>>& map, size_t index, list_iterator_t list_iterator, DB_MUTEX<mutex_t>& mx) : mArray(map), mIndex(index), mListIterator(list_iterator), mMutex(mx)
		{
			mMutex.GetElemRW(mIndex).lock();
			bool is_empty = mIndex != mArray.size() && mArray[mIndex].empty();
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
			if (mIndex == this->mArray.size())
				return mIndex == rh.mIndex;
			return mIndex == rh.mIndex && mListIterator == rh.mListIterator;				
		}
		bool operator!=(const Iterator& rh) { return !operator==(rh); }

		elem_t& operator*(){ return *mListIterator; }

		size_t Index() const { return mIndex; }	
		
		list_iterator_t ListIterator() const { return mListIterator; }


	private:
		void do_increment();
		void do_decrement();
		void seek_prev();

		size_t mIndex = 0;
		std::vector<std::list<elem_t>>& mArray;

		list_iterator_t mListIterator;

		DB_MUTEX<mutex_t>& mMutex;
	};

	Iterator<elem_t> Begin()	{
		return Iterator<elem_t>(mArray, 0, mArray[0].begin(), mMutex);
	}
	Iterator<elem_t> End()	{
		return Iterator<elem_t>(mArray, mArray.size(), mArray[mArray.size() - 1].end(), mMutex);
	}

};

template<class key_t, class mapped_t, class mutex_t>
template<class key_t2>
mapped_t HashTable<key_t, mapped_t, mutex_t>::load(key_t2&& key)
{
	auto lock_guard = std::lock_guard<mutex_t>(get_elem_mx(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it == b_list.end())
	{
		it = b_list.emplace(it, std::forward<key_t>(key), mapped_t());
		mSize.fetch_add(1);
	}
	return it->second;
}

template<class key_t, class mapped_t, class mutex_t>
template<class key_t2, class mapped_t2>
void HashTable<key_t, mapped_t, mutex_t>::store(key_t2&& key, mapped_t2&& value)
{
	auto lock_guard = std::lock_guard<mutex_t>(get_elem_mx(key));

	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it == b_list.end())
	{
		it = b_list.emplace(it, std::forward<key_t>(key), std::forward<mapped_t>(value));
		mSize.fetch_add(1);
	}
	it->second = value;
}

template<class key_t, class mapped_t, class mutex_t>
template<class key_t2>
auto HashTable<key_t, mapped_t, mutex_t>::operator[](key_t2&& key) -> Entry<elem_t>
{
	auto lock_guard = std::lock_guard<mutex_t>(get_elem_mx(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it == b_list.end())
	{
		it = b_list.emplace(it, std::forward<key_t>(key), mapped_t());
		mSize.fetch_add(1);
	}
	return Entry<elem_t>(*it, mMutex);
}

template<class key_t, class mapped_t, class mutex_t>
void HashTable<key_t, mapped_t, mutex_t>::Erase(const key_t& key)
{
	auto lock_guard = std::lock_guard<mutex_t>(get_elem_mx(key));
	auto& b_list = bucket(key);
	auto it = std::find_if(b_list.begin(), b_list.end(), [&key](const elem_t& el){ return el.first == key; });
	if (it != b_list.end())
	{
		b_list.erase(it);
		mSize.fetch_add(-1);
	}
}

template<class key_t, class mapped_t, class mutex_t>
template<class elem_t>
void HashTable<key_t, mapped_t, mutex_t>::Iterator<elem_t>::do_increment()
{

	// CONSIDER GUARDS!!!!!!!!!!
	// in case of incrementing end or decrementing begin

	if (mIndex == mArray.size()) return;

	mMutex.GetElemRW(mIndex).lock();
	auto list_end = mArray[mIndex].end();

	if (mListIterator != list_end)
		++mListIterator;

	if (mListIterator == list_end)
	{
		do	{
			// order matters due to deadlock possibility
			mMutex.GetElemRW(mIndex).unlock();

			if (++mIndex == mArray.size())	return;

			mMutex.GetElemRW(mIndex).lock();
		} while (mArray[mIndex].empty());

		mListIterator = mArray[mIndex].begin();

	}
	// unlock last index mutex, mb original one
	mMutex.GetElemRW(mIndex).unlock();
}

template<class key_t, class mapped_t, class mutex_t>
template<class elem_t>
void HashTable<key_t, mapped_t, mutex_t>::Iterator<elem_t>::do_decrement()
{

	// CONSIDER GUARDS!!!!!!!!!!

	mMutex.GetElemRW(mIndex).lock();

	if (mIndex == mArray.size())
		seek_prev();

	//if (mIndex != mArray.size())
	else
	{
		auto list_begin = mArray[mIndex].begin();
		// assume mListIterator is valid
		if (mListIterator != list_begin)
			--mListIterator;
		else if (!mIndex) return;
		else	seek_prev();
	}
	// unlock last index mutex, mb original one
	mMutex.GetElemRW(mIndex).unlock();
}

// seek for the very previous valid iterator
// unlocks input, locks output
template<class key_t, class mapped_t, class mutex_t>
template<class elem_t>
void HashTable<key_t, mapped_t, mutex_t>::Iterator<elem_t>::seek_prev()
{

	do {
		// order matters due to deadlock possibility
		mMutex.GetElemRW(mIndex).unlock();
		mMutex.GetElemRW(--mIndex).lock();
	} while (mIndex > 0 && mArray[mIndex].empty());

	if (mIndex >= 0 && !mArray[mIndex].empty())
	{
		mListIterator = mArray[mIndex].end();
		--mListIterator;
	}
}
