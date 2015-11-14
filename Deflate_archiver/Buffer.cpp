#include "Buffer.h"

#include <iostream>
#include <string>

#include "MacrosAndPrecomputers.h"
#include "Exceptions.h"

Buffer::Buffer(fstream &s): st(s) {
	pos = 0;
}

InBuffer::InBuffer(fstream &s): Buffer(s) {
	st.get(buf);
}

void InBuffer::buf_refresh() {
	if (!st.is_open()) throw InBufferFail("trying to read from unopened stream");

	st.get(buf);
	//cout << (int) buf << ' ' << st.eof() << endl;
	if (st.fail()) throw InBufferFail("failed to read from stream");

	pos = 0;
}

int InBuffer::readbits(int n) {
	int res = 0;
	for (int i = 0; i < n; ++i) {
		if (pos == 8) {
			buf_refresh();
		}
		SETBIT(res, i, GETBIT(buf, pos++));
	}
	return res;
}

void InBuffer::skipbits() {
	buf_refresh();
}

OutBuffer::OutBuffer(fstream &s): Buffer(s) {
	buf = 0;
}

void OutBuffer::buf_refresh() {
	if (!st.is_open()) throw OutBufferFail("trying to write to unopened stream");

	st.put(buf);
	if (st.fail()) throw OutBufferFail("faild to write to stream");

	buf = 0;
	pos = 0;
}

void OutBuffer::writebits(int val, int n, bool stOrder) {
	for (int i = 0; i < n; ++i) {
		int bit = (stOrder ? i : n - 1 - i);
		if (pos == 8) {
			buf_refresh();
		}
		SETBIT(buf, pos++, GETBIT(val, bit));
	}
}

OutBuffer::~OutBuffer() {
	if (pos)
		buf_refresh();
}