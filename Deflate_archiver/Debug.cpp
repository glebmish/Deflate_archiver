#include "Debug.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <random>
#include <time.h>
#include <string>
#include <set>
using namespace std;

#include "MacrosAndPrecomputers.h"
#include "Buffer.h"
#include "Trie.h"
#include "Window.h"
#include "Huffman and Trie Builder.h"

void buffer_getbits_test() {
	srand((uint)time(NULL));
	fstream file("tmp", ios_base::out | ios_base::binary);
	vector<int> lens;
	vector<int> vals;
	char bf = 0;
	int j = 0;

	//generate file
	for(int i = 0; i <= 100; ++i) {
		int len = rand() % 14 + 1;
		int val = rand() % (1 << len);

		for (int k = 0; k < len; ++k) {
			SETBIT(bf, j++, GETBIT(val, k));
			if (j == 8) {
				file.put(bf);
				j = 0;
				bf = 0;
			}
		}
		lens.push_back(len);
		vals.push_back(val);
	}
	file.put(bf);
	file.close();

	fstream in("tmp", ios_base::in | ios_base::binary);
	InBuffer buf(in);
	bool right = 1;

	//read file
	for (uint i = 0; i < lens.size(); ++i) {
		int cur = buf.readbits(lens[i]);
		right &= (cur == vals[i]);
	}

	if (right) cout << "Buffer::readbits      : Test completed\n";
	else cout << "Buffer::readbits      : Test failed\n";
	remove("tmp");
}

void buffer_test() {
	buffer_getbits_test();
	cout << endl;
}

void window_add_test_adding(int ct, fstream &out) {
	OutWindow slWindow(out);
	for (int i = 0; i < ct; ++i)
		slWindow.add('5');
}

void window_add_test() {
	fstream out("tmp", ios_base::out);
	srand((uint)time(NULL));
	int ct = rand() + 32768;
	
	//generate sliding window
	window_add_test_adding(ct, out);
	out.close();

	fstream check("tmp", ios_base::in);
	int ctCheck = 0;

	//check if number of chars in file equals original
	while(!check.eof()) {
		int tmp;
		tmp = check.get();
		ctCheck++;
	}

	if (ct == ctCheck - 1) cout << "Window::add           : Test completed\n";
	else cout << "Window::add           : Test failed\n" << ct << " " << ctCheck << endl;
	remove("tmp");
}

void window_get_lits_small_test() {
	fstream out("tmp", ios_base::out);
	srand((uint)time(NULL));
	OutWindow windowLess32(out);
	int border = rand() % 32767 + 1; 
	vector<char> realWindow;
	realWindow.clear();

	//generate sliding window size of <32K
	for(int i = 0; i < border; ++i) {
		unsigned char curCh = rand() % 256;
		windowLess32.add(curCh);
		realWindow.push_back(curCh);
	}
	
	//reading from sliding window
	bool right = 1;
	for(int i = 0; i < 100; ++i) {
		int len = rand() % 255 + 3, dst = rand() % (border - 1) + 1;
		string str = windowLess32.add_lits(len, dst);
		string strCheck;
		for (int i = 0; i < len; ++i) {
			strCheck += realWindow[realWindow.size() - dst];
			realWindow.push_back(realWindow[realWindow.size() - dst]);
		}
		right &= (str == strCheck);
	}
	if (right) cout << "Window::get_lits <32K : Test completed\n";
	else cout << "Window::get_lits <32K : Test failed\n";
}

void window_get_lits_big_test() {
	fstream out("tmp", ios_base::out);
	//srand((uint)time(NULL));
	srand(5);
	OutWindow windowMore32(out);
	int border = 32768 + rand() % 16384;
	vector<char> realWindow;
	realWindow.clear();

	//generate sliding window size of >32K

	for(int i = 0; i < border; ++i) {
		char curCh = rand() % 256;
		windowMore32.add(curCh);
		realWindow.push_back(curCh);
	}

	//reading from sliding window
	bool right = 1;
	for(int i = 0; i < 100; ++i) {
		int len = rand() % 255 + 3, dst = rand() % 32768;
		if (len == 91 && dst == 2006)
			cout << "err\n";
		string str = windowMore32.add_lits(len, dst);
		cout << str.size() << endl;
		string strCheck;
		for (int i = 0; i < len; ++i) {
			strCheck += realWindow[realWindow.size() - dst];
			realWindow.push_back(realWindow[realWindow.size() - dst]);
		}
		cout << strCheck.size() << endl;
		//right &= (str == strCheck);
		if (str == strCheck)
			right &= true;
		else {
			right &= false;
			for (int i = 0; i < str.size(); i++) cout << (int) str[i] << " " << (int) strCheck[i] << endl;
		}
	}
	if (right) cout << "Window::get_lits >32K : Test completed\n";
	else cout << "Window::get_lits >32K : Test failed\n";
}

void window_test() {
	window_add_test();
	window_get_lits_small_test();
	window_get_lits_big_test();
	cout << endl;
}

void decoder(vector<int> lens, vector<int> &codes, vector<int> lenCount) {
	int code = 0;
	lenCount[0] = 0;
	vector<int> nextCode(lenCount.size());	
	for (uint bits = 1; bits < lenCount.size(); bits++) {
		code = (code + lenCount[bits - 1]) << 1;
		nextCode[bits] = code;
	}

	for (uint n = 0; n < codes.size(); n++) {
		int len = lens[n];
		if (len != 0) {
			codes[n] = nextCode[len];
			nextCode[len]++;
		}
	}
	remove("tmp");
}

void trie_hc_test() {
	fstream in;
	InBuffer buf(in);
	Trie trie(buf);
	srand((uint)time(NULL));
	vector<int> words;
	vector<int> lens;
	vector<int> lits;

	int order[] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
	11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	//generate lits codes and build trie
	vector<int> hcCode(19, 0);

	for (int i = 0; i <= 18; i++) {
		hcCode[i] = i;
		words.push_back(hcCode[i]);
		lens.push_back(5);
		lits.push_back(i);
		trie.add_word(words[i], lens[i], i);
	}

	//generate coded file
	int ct = 16000;
	int j = 0;
	char bf = 0;
	vector<int> need;
	fstream file("tmp", ios_base::out | ios_base::binary);

	for (int i = 0; i < ct; ++i) {
		int cur = rand() % words.size();

		int word = words[cur], len = lens[cur], lit = lits[cur];

		//write lit code
		for (int k = len - 1; k >= 0; --k) {
			SETBIT(bf, j++, GETBIT(word, k));
			if (j == 8) {
				file.put(bf);
				j = 0;
				bf = 0;
			}
		}

		need.push_back(lit);
	}
	file.put(bf);
	file.close();

	//read file and check
	in.open("tmp", ios_base::in | ios_base::binary);
	bool right = 1;

	for(int i = 0; i < ct; ++i) {
		int word = trie.next_word();
		right &= (word == need[i]);
	}

	if (right) cout << "Trie for hc           : Test completed\n";
	else cout << "Trie for hc           : Test failed\n";
	remove("tmp");
}

void trie_lits_test() {
	fstream in;
	InBuffer buf(in);
	Trie trie(buf);
	srand((uint)time(NULL));
	int extraBits[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
	};
	int startFrom[] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
	};
	vector<int> words;
	vector<int> lens;
	vector<int> lits;

	//generate lits codes and build trie
	vector<int> litLen(286);
	vector<int> litCode(286);
	vector<int> litLenCount(10,0);

	for (int i = 0; i <= 143; i++)
		litLen[i] = 8, lens.push_back(8);
	for (int i = 144; i <= 255; i++)
		litLen[i] = 9, lens.push_back(9);
	for (int i = 256; i <= 279; i++)
		litLen[i] = 7, lens.push_back(7);
	for (int i = 280; i <= 285; i++)
		litLen[i] = 8, lens.push_back(8);
	for (int i = 0; i <= 285; i++) {
		litLenCount[litLen[i]]++;
		lits.push_back(i);
	}
	decoder(litLen, litCode, litLenCount);

	for (int i = 0; i <= 285; i++) {
		words.push_back(litCode[i]);
		trie.add_word(words[i], lens[i], i);
	}

	//generate coded file
	int ct = 16000;
	int j = 0;
	char bf = 0;
	vector<int> need;
	fstream file("tmp", ios_base::out | ios_base::binary);

	for (int i = 0; i < ct; ++i) {
		int cur = rand() % words.size();

		int word = words[cur], len = lens[cur], lit = lits[cur];

		//write lit code
		for (int k = len - 1; k >= 0; --k) {
			SETBIT(bf, j++, GETBIT(word, k));
			if (j == 8) {
				file.put(bf);
				j = 0;
				bf = 0;
			}
		}

		//if lit > 256 write additional code
		int ln = 0;

		if (lit > 256) {
			int add = rand() % (1 << extraBits[lit - 257]);
			if (lit == 284 && add == 31) --add;

			for (int k = 0; k < extraBits[lit - 257]; ++k) {
				SETBIT(bf, j++, GETBIT(add, k));
				if (j == 8) {
					file.put(bf);
					j = 0;
					bf = 0;
				}
			}

			ln = startFrom[lit - 257] + add;
		}

		if (lit <= 256)
			need.push_back(lit);
		else need.push_back(ln);
	}
	file.put(bf);
	file.close();

	//read file and check
	in.open("tmp", ios_base::in | ios_base::binary);
	bool right = 1;

	for(int i = 0; i < ct; ++i) {
		int word = trie.next_word();
		if (word > 256)
			word = litStartFrom[word - 257] + buf.readbits(litExtraBits[word - 257]);
		right &= (word == need[i]);
	}

	if (right) cout << "Trie for lits         : Test completed\n";
	else cout << "Trie for lits         : Test failed\n";
	remove("tmp");
}

void trie_dist_test() {
	fstream in;
	InBuffer buf(in);
	Trie trie(buf);
	srand((uint)time(NULL));
	int extraBits[] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
	};
	int startFrom[] = {
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
	};
	vector<int> words;
	vector<int> lens;
	vector<int> lits;

	//generate lits codes and build trie
	vector<int> dstLen(30, 5);
	vector<int> dstCode(30);

	for (int i = 0; i <= 29; i++) {
		dstCode[i] = i;
		words.push_back(dstCode[i]);
		lens.push_back(5);
		lits.push_back(i);
		trie.add_word(words[i], lens[i], i);
	}

	//generate coded file
	int ct = 16000;
	int j = 0;
	char bf = 0;
	vector<int> need;
	fstream file("tmp", ios_base::out | ios_base::binary);

	for (int i = 0; i < ct; ++i) {
		int cur = rand() % words.size();

		int word = words[cur], len = lens[cur], lit = lits[cur];

		//write lit code
		for (int k = len - 1; k >= 0; --k) {
			SETBIT(bf, j++, GETBIT(word, k));
			if (j == 8) {
				file.put(bf);
				j = 0;
				bf = 0;
			}
		}

		int add = rand() % (1 << extraBits[lit]);

		for (int k = 0; k < extraBits[lit]; ++k) {
			SETBIT(bf, j++, GETBIT(add, k));
			if (j == 8) {
				file.put(bf);
				j = 0;
				bf = 0;
			}
		}
		
		int ln = startFrom[lit] + add;
		need.push_back(ln);
	}
	file.put(bf);
	file.close();

	//read file and check
	in.open("tmp", ios_base::in | ios_base::binary);
	bool right = 1;

	for(int i = 0; i < ct; ++i) {
		int word = trie.next_word();
		word = dstStartFrom[word] + buf.readbits(dstExtraBits[word]);
		right &= (word == need[i]);
	}

	if (right) cout << "Trie for dist         : Test completed\n";
	else cout << "Trie for dist         : Test failed\n";
	remove("tmp");
}

void trie_test() {
	trie_hc_test();
	trie_lits_test();
	trie_dist_test();
	cout << endl;
}

void huf_builder_test() {
	vector<int> vec;
	vec.push_back(5);
	vec.push_back(3);
	vec.push_back(1);
	vec.push_back(1);

	vector<int> len;
	Huffman_builder(vec, len);
	
	for(int i = 0; i < vec.size(); ++i)
		cout << (int)vec[i] << " " << len[i] << endl;

	vector<int> lenCount(5, 0);
	vector<int> code;

	for (int i = 0; i < vec.size(); i++) {
		lenCount[len[i]]++;
	}
	Huffman_decoder(len, code, lenCount);
	for (int i = 0; i < code.size(); ++i)
		cout << i << " " << code[i] << " " << len[i] << endl;

	cout << "Huffman_builder       : Test completed\n";
}
 
void huf_test() {
	huf_builder_test();
	cout << endl;
}

void test() {
	while (true) {
		cout << "1. Full test\n";
		cout << "2. Buffer test\n";
		cout << "3. Window test\n";
		cout << "4. Trie test\n";
		cout << "5. Huffman duilder test\n";
		cout << "\n0. Exit\n\n";
		int t;
		cin >> t;
		switch (t) {
		case 1:
			std::system("cls");
			buffer_test();
			window_test();
			trie_test();
			break;
		case 2:
			std::system("cls");
			buffer_test();
			break;
		case 3:
			std::system("cls");
			window_test();
			break;
		case 4:
			std::system("cls");
			trie_test();
			break;
		case 5:
			std::system("cls");
			huf_test();
			break;
		case 0:
			return;
			break;
		default:
			std::system("cls");
			cout << "Error, try again\n";
			break;
		}
		cout << "Done!\n";
		std::system("pause");
		std::system("cls");
	}
}

void debug() {
	int check = -1;
	while (check == -1) {
		cout << "1. Tests";
		cout << "\n0. Exit\n\n";
		cin >> check;
		switch (check) {
		case 1:
			std::system("cls");
			test();
			check = -1;
			break;
		case 0:
			std::system("cls");
			return;
			break;
		default:
			std::system("cls");
			cout << "Error, try again\n";
			check = -1;
			break;
		}
	}
}