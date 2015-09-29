#include "Trie.h"

#include <iostream>
#include <cstring>

#include "Exceptions.h"
#include "MacrosAndPrecomputers.h"

Trie::Trie (InBuffer &b) : buf(b) {
	for (int i = 0; i < 4; ++i)
		arr[i].resize(1000, 0);
	arrLen = 0;
 }

void Trie::more_space() {
	int newSz = arr[0].size() * 2;
	for (int i = 0; i < 4; ++i)
		arr[i].resize(newSz, 0);
}

void Trie::add_word(int binaryCode, int codeLen, int lit) {
	if (lit < 0 || lit > 287) throw WrongValExc(lit);

	int c = 0;
	for (int i = codeLen - 1; i >= 0; --i) {
		int bit = GETBIT(binaryCode, i);
		
		if (arr[bit][c] > 0)
			c = arr[bit][c];
		else
			c = arr[bit][c] = ++arrLen;

		if (c >= arr[0].size()) more_space();
	}
	arr[2][c] = 1;
	arr[3][c] = lit;
}

/*if code means literal or code length then literal or code length value returns (positive value)
  if it means length or distance then its complement returns (negative value) */
int Trie::next_word() {
    int c = 0;
	int code = 0, j = 0;
    do {
		 int bit = buf.readbits(1);
		 SETBIT(code, j++, bit);

		 if (!arr[bit][c]) 
			 throw WrongCodeExc(code);
		 else c = arr[bit][c];
	} while (!arr[2][c]);
	return arr[3][c];
}

void Trie::write_table() {
	fstream f("file.txt", ios_base::out);
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < arr[i].size(); ++j)
			f << arr[i][j] << " ";
		f << "\n\n\n";
	}
}