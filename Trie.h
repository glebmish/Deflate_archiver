#pragma once


#include <vector>
using namespace std;

#include "Buffer.h"

class Trie {
	vector<int> arr[4];
	int arrLen;
	InBuffer &buf;

	void more_space();

public:
	Trie(InBuffer &b);

	void add_word(int binaryCode, int codeLen, int lit);
	int next_word();

	void write_table();
};