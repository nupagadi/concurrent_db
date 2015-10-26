#include <iostream>
#include <map>
#include <ctime>

#include "HashTable.h"

using namespace std;

int main()
{

	auto threads_num = thread::hardware_concurrency();

	deque<string> deq;
	{
		HashTable<string, string> m(threads_num);

		for (int i = 0; i < 1048576*4; ++i)
		{
			m[to_string(i)] = to_string(i);
			deq.push_back(to_string(i));
		}

		auto job = [&](size_t start){
			volatile size_t s;
			size_t size = 1048576 * 4/threads_num + start;
			for (int i = start; i < size; ++i)
			{
				s = m.load(deq[i]).size();
			}
			for (int i = start; i < size; ++i)
			{
				m.store(deq[i], "qwerty");
			}
		};
		deque<thread> threads;
		
		auto start = time(nullptr);
		for (int i = 0; i < threads_num; ++i)
		{
			threads.emplace_back(job, 1048576 * 4 / threads_num * i);
		}

		for (auto& el : threads)
			el.join();
		auto end = time(nullptr);
		cout << end - start << endl;
	}

	{
		std::map<string, string> m;
		for (int i = 0; i < 1048576* 4; ++i)
			m[to_string(i)] = to_string(i);

		auto start = time(nullptr);
		//for (int i = 0; i < threads_num; ++i)
		{
			volatile size_t s;
			for (int i = 0; i < 1048576 * 4; ++i)
			{
				s = m[deq[i]].size();
			}
			for (int i = 0; i < 1048576 * 4; ++i)
			{
				m[deq[i]] = "qwerty";
			}
		}

		auto end = time(nullptr);
		cout << end - start << endl;
	}






	/*
	h_table[string("121")] = string("121");

	string str = "122";
	h_table[str] = str;

	h_table["12"] = "12";
	h_table["11"] = "11";
	h_table["erase"] = "erase";

	//h_table.Erase("erase");

	//cout << (string)h_table["121"] << endl;


	auto iter = h_table.Begin();
	auto iter2 = h_table.End();
	h_table.Erase(iter2);
	//*iter = make_pair(string("qwe"), string("qwe"));
	cout << (*iter).first << endl
		<< (*iter).second << endl;
	cout << boolalpha;
	cout<< (*iter).first
		<< (*iter).second
		<< (iter == iter2) << endl
		<< (iter != iter2) << endl;
	++iter;
	cout << (*iter).first
		<< (*iter).second
		<< (iter == iter2) << endl
		<< (iter != iter2) << endl;
	++iter;
	cout << (*iter).first
		<< (*iter).second
		<< (iter == iter2) << endl
		<< (iter != iter2) << endl;
	++iter;
	cout << (*iter).first
		<< (*iter).second
		<< (iter == iter2) << endl
		<< (iter != iter2) << endl;
	--iter;
	cout << (*iter).first
		<< (*iter).second
		<< (iter == iter2) << endl
		<< (iter != iter2) << endl;
	--iter;
	cout << (*iter).first
		<< (*iter).second
		<< (iter == iter2) << endl
		<< (iter != iter2) << endl;
	--iter;
	cout << (*iter).first
		<< (*iter).second
		<< (iter == iter2) << endl
		<< (iter != iter2) << endl;




	HashTable<int, string> htint(threads_num);
	htint[0] = "0";
	htint[20] = "20";
	htint[30] = "30";
	cout << (string)htint[0] << endl;
	cout << (string)htint[20] << endl;
	cout << (string)htint[30] << endl;
	cout << (string)htint[40] << endl;

	HashTable<int, int> htint2(threads_num);
	htint2[2] = 3;
	htint2[2] = 2;
	htint2[0] = 0;
	cout << (int)htint2[0] << endl;
	cout << (int)htint2[2] << endl;
	cout << (int)htint2[3] << endl;
	cout << (int)htint2[40] << endl;

	*/
}