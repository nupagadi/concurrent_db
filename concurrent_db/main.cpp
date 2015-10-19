#include <iostream>

#include "HashTable.h"

using namespace std;

int main()
{


	auto threads_num = thread::hardware_concurrency();

	HashTable<string, string> h_table(threads_num);

	h_table[string("121")] = string("121");

	string str = "122";
	h_table[str] = str;

	cout << h_table["121"] << endl;

	auto iter = h_table.Begin();
	auto iter2 = h_table.End();
	++iter;
	--iter;

	HashTable<int, string> htint(threads_num);
	htint[0] = "0";
	htint[20] = "20";
	htint[30] = "30";
	cout << htint[0] << endl;
	cout << htint[20] << endl;
	cout << htint[30] << endl;
	cout << htint[40] << endl;

	HashTable<int, int> htint2(threads_num);
	htint2[2] = 3;
	htint2[2] = 2;
	htint2[0] = 0;
	cout << htint2[0] << endl;
	cout << htint2[2] << endl;
	cout << htint2[3] << endl;
	cout << htint2[40] << endl;
}