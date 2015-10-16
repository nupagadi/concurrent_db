#include <iostream>
#include <deque>
#include <string>
#include <list>


#include "HashTable.h"

using namespace std;

int main()
{
	//auto p = make_pair(string("sdlfkj"), string("sdlffgbdfbdfkj"));
	//list<pair<string, string>> l;
	//cout << sizeof(p) << endl;
	//cout << sizeof(l) << endl;


	HashTable<string, string> h_table;

	h_table[string("121")] = string("121");
	h_table.Set("122", "122");
	h_table.Set("123", "123");
	h_table.Set("124", "124");
	h_table.Set("125", "125");
	h_table.Set("126", "126");
	h_table.Set("127", "127");
	h_table.Set("128", "128");
	h_table.Set("129", "129");
	h_table.Set("120", "120");
	//h_table.Set("11", "11");
	//h_table.Set("12", "12");
	//h_table.Set("13", "13");
	//h_table.Set("14", "14");
	//h_table.Set("15", "15");
	//h_table.Set("16", "16");
	//h_table.Set("17", "17");
	//h_table.Set("18", "18");
	//h_table.Set("19", "19");
	//h_table.Set("10", "10");
	//h_table.Set("21", "21");
	//h_table.Set("22", "22");
	//h_table.Set("23", "23");
	//h_table.Set("24", "24");
	//h_table.Set("25", "25");
	//h_table.Set("26", "26");
	//h_table.Set("27", "27");
	//h_table.Set("28", "28");
	//h_table.Set("29", "29");
	//h_table.Set("20", "20");
	h_table.Set("1", "1");
	h_table.Set("2", "2");
	h_table.Set("3", "3");
	h_table.Set("4", "4");
	h_table.Set("5", "5");
	h_table.Set("6", "6");
	h_table.Set("7", "7");
	h_table.Set("8", "8");
	h_table.Set("9", "9");
	h_table.Set("0", "0");

	cout << h_table.Get("0") << endl;
	cout << h_table.Get("123") << endl;
	cout << h_table.Get("1230") << endl;
	cout << h_table.Get("1") << endl;
	cout << h_table["121"] << endl;
	cout << h_table.Get("1230") << endl;

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