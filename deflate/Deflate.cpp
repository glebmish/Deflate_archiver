#include "Deflate.h"

#include <vector>
#include <cstdlib>

#include "Buffer.h"
#include "Window.h"
#include "Huffman and Trie Builder.h"
#include "MacrosAndPrecomputers.h"

#define DEBUG

//#define MAX_BLOCK_SIZE 819200 //100KB
#define MAX_BLOCK_SIZE 81920 //10KB
#define MAX_STORED_SIZE 65536 //2^16
struct Symb {
	bool isLit;
	int lit;
	int add;
	int addLen;

	Symb(bool il, int l, int a, int al): isLit(il), lit(l), add(a), addLen(al) {}
};

vector<Symb> LZ77_encode_one(int prevLen, int prevDst, string &curStr) {
	vector<Symb> paste;
	if (prevDst == 0) {
		paste.push_back(Symb(true,(unsigned char) curStr[0], 0, 0));
		curStr.assign(curStr.begin() + 1, curStr.end());
	} else {
		int j = 0;
		while (j + 1 <= 28 && prevLen >= litStartFrom[j + 1]) j++;
		int lenAdd = prevLen - litStartFrom[j];
		int lenAddLen = litExtraBits[j];
		
		paste.push_back(Symb(true, j + 257, lenAdd, lenAddLen));
		
		j = 0;
		while (j + 1 <= 29 && prevDst >= dstStartFrom[j + 1]) j++;
		int dstAdd = prevDst - dstStartFrom[j];
		int dstAddLen = dstExtraBits[j];
		
		paste.push_back(Symb(false, j, dstAdd, dstAddLen));

		curStr = curStr[curStr.length() - 1];
	}
	return paste;
}

vector<Symb> LZ77_encode(InWindow &slWindow, int blockSize, vector<Symb> &lz77coded, vector<unsigned char> &charList) {
	bool shouldStore = blockSize <= MAX_STORED_SIZE;
	string curStr = "";
	int prevLen = 0, prevDst = 0;

	for (int i = 0; i < min(blockSize, 3); ++i) {
		unsigned char curChr = slWindow.get();
		if (shouldStore) charList.push_back(curChr);
		lz77coded.push_back(Symb(true, curChr, 0, 0));
	}
	for (int i = 3; i < blockSize; i++) {
		unsigned char curChr = slWindow.get();
		if (shouldStore) charList.push_back(curChr);
		curStr += curChr;
		if (curStr.length() < 3)
			continue;

		int dst;
		if (prevLen == 258)
			dst = -1;
		else
			dst = slWindow.find(curStr, prevDst);

		if (dst == -1) {
			vector<Symb> paste = LZ77_encode_one(prevLen, prevDst, curStr);
			lz77coded.insert(lz77coded.end(), paste.begin(), paste.end());
			prevLen = prevDst = 0;
		} else {
			prevLen = curStr.length();
			prevDst = dst;
		}
	}
	vector<Symb> paste = LZ77_encode_one(prevLen, prevDst, curStr);
	lz77coded.insert(lz77coded.end(), paste.begin(), paste.end());
	prevLen = prevDst = 0;
	lz77coded.push_back(Symb(true, 256, 0, 0));

	return lz77coded;
}

void deflate_stored(OutBuffer &buf, vector<unsigned char> &charList) {
	short BTYPE = 0;
	buf.writebits(BTYPE, 2, true);
	buf.writebits(0, 5, true); //stored bytes should start from byte bound

	short LEN = charList.size();
	buf.writebits(LEN, 16, true);
	buf.writebits(~LEN, 16, true);

	for (int i = 0; i < LEN; ++i)
		buf.writebits(charList[i], 8, true);
}

vector<int> get_codes(vector<int> lens, int maxLen) {
	vector<int> lenCounts(maxLen, 0);
	vector<int> codes;
	for (int i = 0; i < lens.size(); i++)
		lenCounts[lens[i]]++;
	Huffman_decoder(lens, codes, lenCounts);
	return codes;
}

void print_encoded(OutBuffer &buf, vector<Symb> coded, vector<int> litCodes, vector<int> litLens, vector<int> dstCodes, vector<int> dstLens) {
	for (int i = 0; i < coded.size(); i++) {
		Symb cur = coded[i];
		vector<int> &code = (cur.isLit) ? litCodes : dstCodes;
		vector<int> &len = (cur.isLit) ? litLens : dstLens;

		buf.writebits(code[cur.lit], len[cur.lit], false);
		buf.writebits(cur.add, cur.addLen, true);
	}
}

void deflate_fixed(OutBuffer &buf, vector<Symb> &lz77Coded, vector<int> litLens, vector<int> dstLens) {
	short BTYPE = 1;
	buf.writebits(BTYPE, 2, true);

	int maxLen = 20;
	vector<int> litCodes = get_codes(litLens, maxLen);
	vector<int> dstCodes = get_codes(dstLens, maxLen);
	print_encoded(buf, lz77Coded, litCodes, litLens, dstCodes, dstLens);
}

int Hcount(vector<int> lens, int start) {
	int ct = start;
	for (int i = start; i < lens.size(); i++)
		if (lens[i]) ct = i + 1;
	return ct;
}

void print_hc_encoded(OutBuffer &buf, int HCLEN, vector<int> hcCodes, vector<int> hcLens, vector<Symb> coded) {
	for (int i = 0; i < HCLEN; i++)
		buf.writebits(hcLens[hcOrder[i]], 3, true);

	for (int i = 0; i < coded.size(); i++) {
		Symb cur = coded[i];

		buf.writebits(hcCodes[cur.lit], hcLens[cur.lit], false);
		buf.writebits(cur.add, cur.addLen, true);
	}
}

vector<Symb> HC_encode_one(int len, int ct) {
	vector<Symb> paste;
	if (len) {
		paste.push_back(Symb(false, len, 0, 0));
		ct--;
		
		while (ct) {
			int toInsert = min(ct, 6);
			if (toInsert < 3)
				for (int j = 0; j < toInsert; j++)
					paste.push_back(Symb(false, len, 0, 0));
			else
				paste.push_back(Symb(false, 16, toInsert - 3, 2));
			ct -= toInsert;
		}
	} else {
		while (ct) {
			int toInsert = min(ct, 138);
			if (toInsert < 3)
				for (int j = 0; j < toInsert; j++)
					paste.push_back(Symb(false, 0, 0, 0));
			else {
				if (toInsert <= 10)
					paste.push_back(Symb(false, 17, toInsert - 3, 3));
				else 
					paste.push_back(Symb(false, 18, toInsert - 11, 7));
			}
			ct -= toInsert;
		}
	}
	return paste;
}

vector<Symb> HC_encode(int HLIT, int HDIST, vector<int> litLens, vector<int> dstLens) {
	vector<Symb> coded;
	int len = litLens[0];
	int ct = 1;
	for (int i = 1; i < HLIT; i++) {
		if (litLens[i] == len)
			ct++;
		else {
			vector<Symb> paste = HC_encode_one(len, ct);
			coded.insert(coded.end(), paste.begin(), paste.end());
			len = litLens[i];
			ct = 1;
		}
	}
	for (int i = 0; i < HDIST; i++) {
		if (dstLens[i] == len)
			ct++;
		else {
			vector<Symb> paste = HC_encode_one(len, ct);
			coded.insert(coded.end(), paste.begin(), paste.end());
			len = dstLens[i];
			ct = 1;
		}
	}
	vector<Symb> paste = HC_encode_one(len, ct);
	coded.insert(coded.end(), paste.begin(), paste.end());
	return coded;
}

void deflate_dynamic(OutBuffer &buf, vector<Symb> &lz77Coded, vector<int> &litLens, vector<int> &dstLens) {
	short BTYPE = 2;
	buf.writebits(BTYPE, 2, true);

	int HLIT = Hcount(litLens, 257);
	int HDIST = Hcount(dstLens, 1);

	vector<Symb> HCcoded = HC_encode(HLIT, HDIST, litLens, dstLens);

	vector<int> hcProbs(19, 0);
	for (int i = 0; i < HCcoded.size(); i++)
		hcProbs[HCcoded[i].lit]++;

	vector<int> hcLens(19);
	Huffman_builder(hcProbs, hcLens);
	vector<int> hcCodes = get_codes(hcLens, 8);

	int HCLEN = 4;
	for (int i = 4; i < hcLens.size(); i++)
		if (hcLens[hcOrder[i]]) HCLEN = i + 1;

	buf.writebits(HLIT - 257, 5, true);
	buf.writebits(HDIST - 1, 5, true);
	buf.writebits(HCLEN - 4, 4, true);

	print_hc_encoded(buf, HCLEN, hcCodes, hcLens, HCcoded);

	int maxLen = 20;
	vector<int> litCodes = get_codes(litLens, maxLen);
	vector<int> dstCodes = get_codes(dstLens, maxLen);
	print_encoded(buf, lz77Coded, litCodes, litLens, dstCodes, dstLens);
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

int count_len(vector<int> probs, vector<int> lens) { 
	int ct = 0;
	for (int i = 0; i < lens.size(); i++)
		ct += probs[i] * lens[i];
	return ct;
}

void deflate_tree(InWindow &slWindow, OutBuffer &buf, int blockSize) {
	int storedBlockLen = blockSize * 8;

	vector<Symb> lz77coded;
	vector<unsigned char> charList;
	LZ77_encode(slWindow, blockSize, lz77coded, charList);

	vector<int> litProbs(288, 0);
	vector<int> dstProbs(32, 0);
	for (int i = 0; i < lz77coded.size(); i++) {
		if (lz77coded[i].isLit)
			litProbs[lz77coded[i].lit]++;
		else
			dstProbs[lz77coded[i].lit]++;
	}

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

	int fixedBlockLen = count_len(litProbs, fixedLitLen) + count_len(dstProbs, fixedDstLen);

	vector<int> dynamicLitLen(288);
	Huffman_builder(litProbs, dynamicLitLen);
	vector<int> dynamicDstLen(32);
	Huffman_builder(dstProbs, dynamicDstLen);

	int dynamicBlockLen = deflate_dynamic_estimate(dynamicLitLen, dynamicDstLen) +
						  count_len(litProbs, dynamicLitLen) + count_len(dstProbs, dynamicDstLen);;

	if (blockSize <= MAX_STORED_SIZE && storedBlockLen <= fixedBlockLen && storedBlockLen <= dynamicBlockLen)
		deflate_stored(buf, charList);
	else if (fixedBlockLen <= dynamicBlockLen)
		deflate_fixed(buf, lz77coded, fixedLitLen, fixedDstLen);
	else 
		deflate_dynamic(buf, lz77coded, dynamicLitLen, dynamicDstLen);
}

void deflate(fstream &in, fstream &out) {
	InWindow slWindow(in);
	OutBuffer buf(out);

	in.seekg(0, in.end);
	int size = in.tellg();
	in.seekg(0, in.beg);

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
	buf.~OutBuffer();
}

	