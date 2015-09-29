#include "Window.h"

#include <iostream>

#include "Exceptions.h"

Window::Window(fstream &s): st(s) {
	arr = "";
	pos = 0;
	wMax = 32768;
	maxReached = false;
}

OutWindow::OutWindow(fstream &s): Window(s) {}

void OutWindow::add(char lit) {
	arr.push_back(lit);
	++pos;
	if (pos == wMax)
		win_refresh();
}

void OutWindow::win_refresh() {
	st.write(arr.c_str(), pos);
	pos = 0;
	maxReached = true;
}

string OutWindow::add_lits(int len, int dst) {
	string res;
	if (!maxReached && dst > pos || dst > wMax) {//then returns string with len chars of '?'
		for (int i = 0; i < len; ++i) {
			res += '?';
			add('?');
		}
	} else {
		for (int i = 0; i < len; ++i) {
			int curCh = (pos + wMax - dst) % wMax;
			res += arr[curCh];
			add(arr[curCh]);
		}
	}
	
	return res;
}

OutWindow::~OutWindow() {
	win_refresh();
}

InWindow::InWindow(fstream &s): Window(s) {}

char InWindow::get() {
	char c = st.get();
	arr.push_back(c);
	++pos;
	if (pos == wMax)
		win_refresh();
	return c;
}

int InWindow::find(string s) {
	arr.append(s);
	int dst = arr.rfind(s, pos - 1);
	if (dst == string::npos)
		return 0;

	dst = pos - dst;
	string rest = "";
	for (int i = wMax + 1; i < arr.size();) {
		rest += arr[i];
		arr.pop_back();
	}

	return dst;
}

void InWindow::win_refresh() {
	pos = 0;
	maxReached = true;
}