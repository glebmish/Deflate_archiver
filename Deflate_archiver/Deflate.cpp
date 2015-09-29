#include "Deflate.h"

#include <vector>

#include "Buffer.h"
#include "Window.h"
#include "Huffman and Trie Builder.h"
#include "MacrosAndPrecomputers.h"

struct Symb {
	bool isLit;
	int lit;
	int add;
	int addLen;

	Symb(bool il, int l, int a, int al): isLit(il), lit(l), add(a), addLen(al) {}
};

vector<Symb> LZ77(InWindow &slWindow, OutBuffer &buf, int blockSize) {
	vector<Symb> lz77Coded;
	string cur = "";
	int prevLen, prevDst;

	for (int i = 0; i < blockSize; i++) {
		cur += slWindow.get();
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

void deflate_stored(InWindow &slWindow, OutBuffer &buf, int blockSize) {
	short BTYPE = 0;
	buf.writebits(BTYPE, 2, true);
	buf.writebits(0, 5, true); //stored bytes should start from byte bound

	short LEN = blockSize;
	buf.writebits(LEN, 16, true);
	buf.writebits(~LEN, 16, true);

	for (int i = 0; i < blockSize; ++i) {
		char tmp = slWindow.get();
		buf.writebits(tmp, 8, true);
	}
}

void deflate_tree(InWindow &slWindow, OutBuffer &buf, int blockSize) {
	//получение списка lit/len и dst кодов в порядке кодирования
	vector<Symb> lz77Coded = LZ77(slWindow, buf, blockSize);

	//рассчитывание вероятности появления каждого символа для каждого дерева
	vector<int> litProb(288, 0);
	vector<int> dstProb(32, 0);

	for (int i = 0; i < lz77Coded.size(); i++) {
		if (lz77Coded[i].isLit)
			litProb[lz77Coded[i].lit]++;
		else
			dstProb[lz77Coded[i].lit]++;
	}

	//получение длин кодов символов
	vector<int> litLen(287);
	Huffman_builder(litProb, litLen);
	vector<int> dstLen(32);
	Huffman_builder(dstProb, dstLen);

	//заполнение длин кодов символов в фиксированном варианте
	vector<int> fixedLitLen(287);
	for (int i = 0; i <= 143; i++)
		fixedLitLen[i] = 8;
	for (int i = 144; i <= 255; i++)
		fixedLitLen[i] = 9;
	for (int i = 256; i <= 279; i++)
		fixedLitLen[i] = 7;
	for (int i = 280; i <= 287; i++)
		fixedLitLen[i] = 8;
	vector<int> fixedDstLen(32, 5);

	//оценка разницы в длине кодов символа по формуле сумма разниц / кол-во элементов
	//если разница в процентах больше 15 процентов, то строим по полученному дереву, иначе по стандартному 
	int litDif = 0;
	for (int i = 0; i <= 287; i++) {
		litDif += abs(litLen[i] - fixedLitLen[i]);
	}
	double dstDif = 0;
	for (int i = 0; i <= 32; i++) {
		dstDif += abs(dstLen[i] - fixedDstLen[i]);
	}
	double dif = (litDif / 288 + dstDif / 33) / 2;

	vector<int> &realLitLen = (dif > 5) ? litLen : fixedLitLen;
	vector<int> &realDstLen = (dif > 5) ? dstLen : fixedDstLen;

	//получение кодов символов
	vector<int> litLenCount(20, 0);
	vector<int> litCode;
	for (int i = 0; i < realLitLen.size(); i++)
		litLenCount[realLitLen[i]]++;
	Huffman_decoder(realLitLen, litCode, litLenCount);

	vector<int> dstLenCount(20, 0);
	vector<int> dstCode;
	for (int i = 0; i < realDstLen.size(); i++)
		dstLenCount[realDstLen[i]]++;
	Huffman_decoder(realDstLen, dstCode, dstLenCount);

	for (int i = 0; i < lz77Coded.size(); i++) {
		Symb cur = lz77Coded[i];
		vector<int> &code = (cur.isLit) ? litCode : dstCode;
		vector<int> &len = (cur.isLit) ? realLitLen : realDstLen;
		buf.writebits(code[cur.lit], len[cur.lit], false);
		buf.writebits(cur.add, cur.addLen, true);
	}
}

void deflate(fstream &in, fstream &out, int filesize) {
	InWindow slWindow(in);
	OutBuffer buf(out);

	short BFINAL = 1;
	buf.writebits(BFINAL, 1, true);

	if (filesize <= 1024)
		deflate_stored(slWindow, buf, filesize);
	else
		deflate_tree(slWindow, buf, filesize);
}

	