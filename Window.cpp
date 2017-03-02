#include "Window.h"

#include "Exceptions.h"

Window::Window(fstream &s): st(s) {
	arr = "";
	pos = 0;
	wMax = 32768;
	maxReached = false;
}

OutWindow::OutWindow(fstream &s): Window(s) {}

void OutWindow::add(char lit) {
	if (pos >= arr.size())
		arr.push_back(lit);
	else arr[pos] = lit;
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
	if (pos >= arr.size())
		arr.push_back(c);
	else arr[pos] = c;
	++pos;
	if (pos == wMax)
		win_refresh();
	return c;
}

int InWindow::find(string s, unsigned prevDst) {
	if (prevDst != 0 && arr[(pos - prevDst - 1 + wMax) % wMax] == s[s.length() - 1])
		return prevDst;

	int maxBeg = max(0, int(pos - s.length() - 1));
	unsigned dst = arr.rfind(s, maxBeg);
	if (maxReached && dst == string::npos)
		dst = arr.rfind(s);
	if (dst == string::npos)
		return -1;

	dst = (pos - s.length() - dst + wMax) % wMax;
	if (dst == 0)
		return -1;
	return dst;
}

void InWindow::win_refresh() {
	pos = 0;
	maxReached = true;
}