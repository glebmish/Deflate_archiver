#pragma once

#include <fstream>
using namespace std;

class Buffer {
protected:
	Buffer(fstream &s);

	fstream &st;
	char buf;
	int pos;
};

class InBuffer : Buffer {
	void buf_refresh();
public:
	InBuffer(fstream &ff);

	int readbits(int n);
	void skipbits();
};

class OutBuffer : Buffer {
	void buf_refresh();
public:
	OutBuffer(fstream &ff);

	void writebits(int val, int n, bool stOrder);

	~OutBuffer();
};