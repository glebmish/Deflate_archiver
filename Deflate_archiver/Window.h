#pragma once

#include <fstream>
#include <string>
using namespace std;

class Window {
protected:
	string arr;
	int wMax;
	int pos;
	fstream &st;
	bool maxReached;

	Window(fstream &s);
};

class OutWindow : Window {
	void win_refresh();
public:
	OutWindow(fstream &s);

	void add(char lit);
	string add_lits(int len, int dst);

	~OutWindow();	
};

class InWindow : Window {
	void win_refresh();
public:
	InWindow(fstream &s);

	char get();
	int find(string s, unsigned lastDst);
};
	