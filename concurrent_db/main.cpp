#include <iostream>
#include <deque>
#include <string>
#include <list>


#include "HashTable.h"

using namespace std;

int main()
{

	HashTable<string, string> h_table;

	h_table[string("121")] = string("121");

	cout << h_table["121"] << endl;

	HashTable<int, string> htint;
	htint[0] = "0";
	htint[20] = "20";
	htint[30] = "30";
	cout << htint[0] << endl;
	cout << htint[20] << endl;
	cout << htint[30] << endl;
	cout << htint[40] << endl;

	HashTable<int, int> htint2;
	htint2[2] = 3;
	htint2[2] = 2;
	htint2[0] = 0;
	cout << htint2[0] << endl;
	cout << htint2[2] << endl;
	cout << htint2[3] << endl;
	cout << htint2[40] << endl;
}