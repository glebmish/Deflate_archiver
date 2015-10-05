#include "Deflate.h"

#include <vector>

#include "Buffer.h"
#include "Window.h"
#include "Huffman and Trie Builder.h"
#include "MacrosAndPrecomputers.h"

#define MAX_BLOCK_SIZE 83886080 //10MB
#define MAX_STORED_SIZE 65536 //2^16

struct Symb {
	bool isLit;
	int lit;
	int add;
	int addLen;

	Symb(bool il, int l, int a, int al): isLit(il), lit(l), add(a), addLen(al) {}
};

vector<Symb> LZ77(InWindow &slWindow, int blockSize, vector<Symb> &lz77Coded, vector<char> &charList) {
	bool shouldStore = blockSize <= MAX_STORED_SIZE;
	string cur = "";
	int prevLen, prevDst;

	for (int i = 0; i < blockSize; i++) {
		char tmp = slWindow.get();
		if (shouldStore) charList.push_back(tmp);
		cur += tmp;
		if (cur.length() < 3)
			continue;
		
		int dst = slWindow.find(cur);

		if (!dst) {

			if (prevDst == 0) {
				for (int i = 0; i < cur.length(); i++) 
					lz77Coded.push_back(Symb(true, cur[i], 0, 0));
			} else {
				int j = 0;
				while (prevLen > litStartFrom[j + 1]) j++;
				int lenAdd = prevLen - litStartFrom[j];
				int lenAddLen = litExtraBits[j];

				lz77Coded.push_back(Symb(true, j + 257, lenAdd, lenAddLen));
				
				j = 0;
				while (prevDst > dstStartFrom[j + 1]) j++;
				int dstAdd = prevDst - dstStartFrom[j];
				int dstAddLen = dstExtraBits[j];

				lz77Coded.push_back(Symb(false, j, dstAdd, dstAddLen));
			}

			cur = "";
			prevLen = prevDst = 0;

		} else {
			prevLen = cur.length();
			prevDst = dst;
		}
	}

	return lz77Coded;
}

void deflate_stored(OutBuffer &buf, vector<char> &charList) {
	short BTYPE = 0;
	buf.writebits(BTYPE, 2, true);
	buf.writebits(0, 5, true); //stored bytes should start from byte bound

	short LEN = charList.size();
	buf.writebits(LEN, 16, true);
	buf.writebits(~LEN, 16, true);

	for (int i = 0; i < LEN; ++i)
		buf.writebits(charList[i], 8, true);
}

void deflate_fixed(OutBuffer &buf, vector<Symb> &lz77Coded, vector<int> litLen, vector<int> dstLen) {
	//получение кодов символов
	vector<int> litLenCount(20, 0);
	vector<int> litCode;
	for (int i = 0; i < litLen.size(); i++)
		litLenCount[litLen[i]]++;
	Huffman_decoder(litLen, litCode, litLenCount);

	vector<int> dstLenCount(20, 0);
	vector<int> dstCode;
	for (int i = 0; i < dstLen.size(); i++)
		dstLenCount[dstLen[i]]++;
	Huffman_decoder(dstLen, dstCode, dstLenCount);


	//вывод
	for (int i = 0; i < lz77Coded.size(); i++) {
		Symb cur = lz77Coded[i];
		vector<int> &code = (cur.isLit) ? litCode : dstCode;
		vector<int> &len = (cur.isLit) ? litLen : dstLen;
		buf.writebits(code[cur.lit], len[cur.lit], false);
		buf.writebits(cur.add, cur.addLen, true);
	}
}

void deflate_dynamic(OutBuffer &buf, vector<Symb> &lz77Coded, vector<int> litLen, vector<int> dstLen) {
	//подсчет какие символы и сколько их стоит подряд
	vector<pair<int, int> > lenCt; //first - length, second - how much elements of this length in a raw
	int len = litLen[0];
	int ct = 1;
	
	for (int i = 0; i < litLen.size(); i++) {
		if (litLen[i] == len)
			ct++;
		else {
			lenCt.push_back(make_pair(len, ct));
			len = litLen[i];
			ct = 1;
		}
	}
	for (int i = 0; i < dstLen.size(); i++) {
		if (dstLen[i] == len)
			ct++;
		else {
			lenCt.push_back(make_pair(len, ct));
			len = dstLen[i];
			ct = 1;
		}
	}
	lenCt.push_back(make_pair(len, ct));

	//составление последовательности кодированных символов
	vector<Symb> codeAdd; //first - code 0..18, second - additional bits for this code
	for (int i = 0; i < lenCt.size(); i++) {
		if (lenCt[i].first) {
			codeAdd.push_back(Symb(false, lenCt[i].first, 0, 0));
			lenCt[i].second--;

			while (lenCt[i].second) {
				int toInsert = min(lenCt[i].second, 6);
				if (toInsert < 3)
					for (int j = 0; j < toInsert; j++)
						codeAdd.push_back(Symb(false, lenCt[i].first, 0, 0));
				else
					codeAdd.push_back(Symb(false, 16, toInsert - 3, 2));
				lenCt[i].second -= toInsert;
			}
		} else {
			while (lenCt[i].second) {
				int toInsert = min(lenCt[i].second, 138);
				if (toInsert < 3)
					for (int j = 0; j < toInsert; j++)
						codeAdd.push_back(Symb(false, 0, 0, 0));
				else {
					if (toInsert <= 10)
						codeAdd.push_back(Symb(false, 17, toInsert - 3, 3));
					else 
						codeAdd.push_back(Symb(false, 18, toInsert - 11, 7));
				}
				lenCt[i].second -= toInsert;
			}
		}
	}

	//рассчитывание вероятности появления каждого символа
	vector<int> hcProb(19, 0);

	for (int i = 0; i < codeAdd.size(); i++)
		hcProb[codeAdd[i].lit]++;

	//получение длин кодов hc кода
	vector<int> hcLen(19);
	Huffman_builder(hcProb, hcLen);

	//получение кодов hc кода
	vector<int> hcLenCount(8, 0);
	vector<int> hcCode;
	for (int i = 0; i < hcLen.size(); i++)
		hcLenCount[hcLen[i]]++;
	Huffman_decoder(hcLen, hcCode, hcLenCount);

	//вывод кодированного алфавита
	for (int i = 0; i < codeAdd.size(); i++) {
		Symb cur = codeAdd[i];

		buf.writebits(hcCode[cur.lit], hcLen[cur.lit], false);
		buf.writebits(cur.add, cur.addLen, true);
	}

	//получение кодов символов
	vector<int> litLenCount(20, 0);
	vector<int> litCode;
	for (int i = 0; i < litLen.size(); i++)
		litLenCount[litLen[i]]++;
	Huffman_decoder(litLen, litCode, litLenCount);

	vector<int> dstLenCount(20, 0);
	vector<int> dstCode;
	for (int i = 0; i < dstLen.size(); i++)
		dstLenCount[dstLen[i]]++;
	Huffman_decoder(dstLen, dstCode, dstLenCount);


	//вывод кодированных символов
	for (int i = 0; i < lz77Coded.size(); i++) {
		Symb cur = lz77Coded[i];
		vector<int> &code = (cur.isLit) ? litCode : dstCode;
		vector<int> &len = (cur.isLit) ? litLen : dstLen;
		buf.writebits(code[cur.lit], len[cur.lit], false);
		buf.writebits(cur.add, cur.addLen, true);
	}
}

// suppose dynamic code represented as (codesAm * 3) + (codesCt) * 3 * 2
// where codesAm - alphabet for HC codes
//       codesCt - number of switches of length (count 2 if switches on 0)
//       3 - length of one code, 2 - rough estimated length of additional info for each code
int deflate_dynamic_estimate(vector<int> litLen, vector<int> dstLen) {
	int codesCt = 0;
	int codesAm = 0;
	int cur = -1;
	vector<bool> was(19, false);

	for (int i = 0; i < litLen.size(); ++i) {
		if (litLen[i] != cur) {
			if (!was[litLen[i]]) {
				was[litLen[i]] = true;
				codesAm++;
			}
			if (litLen[i] == 0)
				codesCt++;
			codesCt++;
			cur = litLen[i];
		}
	}
	for (int i = 0; i < dstLen.size(); ++i) {
		if (dstLen[i] != cur) {
			if (!was[dstLen[i]]) {
				was[dstLen[i]] = true;
				codesAm++;
			}
			if (dstLen[i] == 0)
				codesCt++;
			codesCt++;
			cur = dstLen[i];
		}
	}
	return codesAm * 3 + codesCt * 3 * 2;
}

void deflate_tree(InWindow &slWindow, OutBuffer &buf, int blockSize) {
	//рассчет длины блока stored
	int storedBlockLen = blockSize * 8 + 4;

	//получение списка lit/len и dst кодов в порядке кодирования
	vector<Symb> lz77Coded;
	vector<char> charList;
	LZ77(slWindow, blockSize, lz77Coded, charList);

	//рассчитывание вероятности появления каждого символа для каждого дерева
	vector<int> litProb(288, 0);
	vector<int> dstProb(32, 0);

	for (int i = 0; i < lz77Coded.size(); i++) {
		if (lz77Coded[i].isLit)
			litProb[lz77Coded[i].lit]++;
		else
			dstProb[lz77Coded[i].lit]++;
	}

	//заполнение длин кодов символов fixed
	vector<int> fixedLitLen(288);
	for (int i = 0; i <= 143; i++)
		fixedLitLen[i] = 8;
	for (int i = 144; i <= 255; i++)
		fixedLitLen[i] = 9;
	for (int i = 256; i <= 279; i++)
		fixedLitLen[i] = 7;
	for (int i = 280; i <= 287; i++)
		fixedLitLen[i] = 8;
	vector<int> fixedDstLen(32, 5);

	//рассчет длины блока fixed
	int fixedBlockLen = 0;
	for (int i = 0; i < fixedLitLen.size(); i++)
		fixedBlockLen += litProb[i] * fixedLitLen[i];
	for (int i = 0; i < fixedDstLen.size(); i++)
		fixedBlockLen += dstProb[i] * fixedDstLen[i];

	//получение длин кодов символов dynamic
	vector<int> dynamicLitLen(288);
	Huffman_builder(litProb, dynamicLitLen);
	vector<int> dynamicDstLen(32);
	Huffman_builder(dstProb, dynamicDstLen);

	//рассчет длины dynamic
	int dynamicBlockLen = deflate_dynamic_estimate(dynamicLitLen, dynamicDstLen);
	for (int i = 0; i < dynamicLitLen.size(); i++)
		dynamicBlockLen += litProb[i] * dynamicLitLen[i];
	for (int i = 0; i < dynamicDstLen.size(); i++)
		dynamicBlockLen += dstProb[i] * dynamicDstLen[i];

	//выбор способа кодирования
	if (blockSize <= MAX_STORED_SIZE && blockSize < storedBlockLen <= fixedBlockLen && storedBlockLen <= dynamicBlockLen)
		deflate_stored(buf, charList);
	else if (fixedBlockLen <= dynamicBlockLen)
		deflate_fixed(buf, lz77Coded, fixedLitLen, fixedDstLen);
	else 
		deflate_dynamic(buf, lz77Coded, dynamicLitLen, dynamicDstLen);
}

void deflate(fstream &in, fstream &out) {
	InWindow slWindow(in);
	OutBuffer buf(out);

	in.seekg(0, in.end);
	int size = in.tellg();
	in.seekg(0, in.beg);

	//считаем длину файла и все такое
	do {
		short BFINAL;
		if (size > MAX_BLOCK_SIZE)
			BFINAL = 0;
		else 
			BFINAL = 1;
		buf.writebits(BFINAL, 1, true);

		int blockSize = min(size, MAX_BLOCK_SIZE);
		deflate_tree(slWindow, buf, blockSize);
		size -= blockSize;
	} while (size);
}

	