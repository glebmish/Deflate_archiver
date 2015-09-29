#include "Inflate.h"

#include <cstring>
#include <string>
#include <vector>

#include "Huffman and Trie Builder.h"
#include "MacrosAndPrecomputers.h"
#include "Buffer.h"
#include "Window.h"
#include "Trie.h"
#include "Debug.h"
#include "Exceptions.h"

void inflate_decode(InBuffer &buf, OutWindow &slWindow, Trie &litTrie, Trie &dstTrie) {
	for (;;) {
		int lit;
		try {
			lit = litTrie.next_word();
		} catch(WrongCodeExc e) {
			e.what();
			lit = '?';
		}
		if (lit == 256)
			return; //block ended
		if (lit >= 0 && lit <=255)
			slWindow.add(lit);
		else {
			int len = litStartFrom[lit - 257] + buf.readbits(litExtraBits[lit - 257]);

			int dst;
			try {
				dst = dstTrie.next_word();
			} catch(WrongCodeExc e) {
				cout << "dst: ";
				e.what();
				dst = 0;
			}
			dst = dstStartFrom[dst] + buf.readbits(dstExtraBits[dst]);

			string lits = slWindow.add_lits(len, dst);
		}
	}
}

void inflate_stored(InBuffer &buf, OutWindow &slWindow) {
	buf.skipbits();
	short LEN = buf.readbits(16);
	short NLEN = buf.readbits(16);
	if (LEN != ~NLEN) throw InflateDecodeFail("length data corrupted");
	while (LEN--) {
		char lit = buf.readbits(8);
		slWindow.add(lit);
	}
}

void inflate_fixed(InBuffer &buf, OutWindow &slWindow) {
	vector<int> litLen(288);
	vector<int> litCode(288);
	vector<int> litLenCount(10,0);

	for (int i = 0; i <= 143; i++)
		litLen[i] = 8;
	for (int i = 144; i <= 255; i++)
		litLen[i] = 9;
	for (int i = 256; i <= 279; i++)
		litLen[i] = 7;
	for (int i = 280; i <= 287; i++)
		litLen[i] = 8;
	for (int i = 0; i <= 287; i++)
		litLenCount[litLen[i]]++;

	Huffman_decoder(litLen, litCode, litLenCount);
	Trie litTrie = build_trie(buf, litCode, litLen);
	litTrie.write_table();

	vector<int> dstLen(32, 5);
	vector<int> dstCode(32);

	for (int i = 0; i <= 31; i++)
		dstCode[i] = i;

	Trie dstTrie = build_trie(buf, dstCode, dstLen);

	inflate_decode(buf, slWindow, litTrie, dstTrie);
}

void inflate_dynamic(InBuffer &buf, OutWindow &slWindow) {
	int HLIT = buf.readbits(5) + 257; 
	int HDIST = buf.readbits(5) + 1;
	int HCLEN = buf.readbits(4) + 4;
	vector<int> hcLen(19, 0);
	vector<int> hcCode(19, 0);
	vector<int> hcLenCount(8, 0);

	for (int i = 0; i < HCLEN; ++i) {
		hcLen[hcOrder[i]] = buf.readbits(3);
		hcLenCount[hcLen[hcOrder[i]]]++;
	}
	

	Huffman_decoder(hcLen, hcCode, hcLenCount);
	Trie hcTrie = build_trie(buf, hcCode, hcLen);

	vector<int> litLen(288, 0);
	vector<int> litCode(288);
	vector<int> litLenCount(288,0);

	int currentCL, timesCounter = 0;

	for (int i = 0; i < HLIT; ++i) {
		//decoding
		if (!timesCounter) {
			int cur = hcTrie.next_word();
			if (cur < 16) {
				timesCounter = 1;
				currentCL = cur;
			} else if (cur == 16) {
				timesCounter = buf.readbits(2) + 3;
			} else if (cur == 17) {
				timesCounter = buf.readbits(3) + 3;
				currentCL = 0;
			} else if (cur == 18) {
				timesCounter = buf.readbits(7) + 11;
				currentCL = 0;
			} 
		}
		timesCounter--;
		litLen[i] = currentCL;
		litLenCount[litLen[i]]++;
	}

	vector<int> dstLen(32, 0);
	vector<int> dstCode(32);
	vector<int> dstLenCount(32,0);

	for (int i = 0; i < HDIST; ++i) {
		//decoding
		if (!timesCounter) {
			int cur = hcTrie.next_word();
			if (cur < 16) {
				timesCounter = 1;
				currentCL = cur;
			} else if (cur == 16) {
				timesCounter = buf.readbits(2) + 3;
			} else if (cur == 17) {
				timesCounter = buf.readbits(3) + 3;
				currentCL = 0;
			} else if (cur == 18) {
				timesCounter = buf.readbits(7) + 11;
				currentCL = 0;
			}
		}
		timesCounter--;
		dstLen[i] = currentCL;
		dstLenCount[dstLen[i]]++;
	}

	Huffman_decoder(litLen, litCode, litLenCount);
	Trie litTrie = build_trie(buf, litCode, litLen);

	Huffman_decoder(dstLen, dstCode, dstLenCount);
	Trie dstTrie = build_trie(buf, dstCode, dstLen);

	inflate_decode(buf, slWindow, litTrie, dstTrie);
}

void inflate(fstream &in, fstream &out) {
	try {
		InBuffer buf(in);
		OutWindow slWindow(out);

		short BFINAL;
		short BTYPE;
		do {
			BFINAL = buf.readbits(1);
			BTYPE = buf.readbits(2);

			if (BTYPE == 0)
				inflate_stored(buf, slWindow);
			if (BTYPE == 1)
				inflate_fixed(buf, slWindow);
			if (BTYPE == 2)
				inflate_dynamic(buf, slWindow);
			if (BTYPE == 3) 
				throw InflateDecodeFail("error defining type of block");
		} while (!BFINAL);
	}
	catch (InBufferFail e) {
		e.what();
		throw InflateDecodeFail("input buffer fail");
	}
}