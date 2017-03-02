#pragma once

#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

//Main.cpp
struct OutOfBoundExc : public exception {
	void what() throw () {
		cout << "Out of bound\n";
	}

	~OutOfBoundExc() throw () {}
};

struct ArchAndDearchExc : public exception {
	void what() throw () {
		cout << "Can't archive and dearchive files at the same time\n";
	}

	~ArchAndDearchExc() throw () {}
};

//gzip.cpp

struct InputOpenFail : public exception {
	string filename;

	InputOpenFail(string f): filename(f) {};

	void what()  throw () {
		cout << "File \"" << filename << "\" could not be opened for reading\n";
		std::system("pause");
	}

	~InputOpenFail() throw () {}
};

struct WrongSizeExc {
	int origSz;
	int curSz;

	WrongSizeExc(int o, int c): origSz(o), curSz(c) {};

	void what() throw () {
		cout << "Original size " << origSz << " does not match with current size " << curSz << ".\n";
		std::system("pause");
	}

	~WrongSizeExc() throw () {}
};

struct WrongCRCExc {
	int origCRC;
	int curCRC;

	WrongCRCExc(int o, int c): origCRC(o), curCRC(c) {};

	void what() throw () {
		cout << "Original CRC32 " << origCRC << " does not match with current CRC32 " << curCRC << ".\n";
		std::system("pause");
	}

	~WrongCRCExc() throw () {}
};

//Inflate.cpp
struct InflateDecodeFail : public exception {
	string error;

	InflateDecodeFail(string e): error(e) {};

	void what() throw () {
		cout << "Failed to decode current file: " << error << ".\n";
		std::system("pause");
	}

	~InflateDecodeFail() throw () {}
};

//Trie.cpp
struct WrongValExc : public exception {
	int val;

	WrongValExc(int v): val(v) {};

	void what() throw () {
		cout << "Wrong literal value " << val << ".\n";
		std::system("pause");
	}

	~WrongValExc() throw () {}
};

struct WrongCodeExc : public exception {
	int code;

	WrongCodeExc(int c): code(c) {};

	void what() throw () {
		cout << "Code " << code << " doesn't exist in the tree\n";
		std::system("pause");
	}

	~WrongCodeExc() throw () {}
};

//Buffer.cpp
struct InBufferFail : public exception {
	string error;

	InBufferFail(string e): error(e) {};

	void what() throw () {
		cout << "Reading fail: "<< error << ".\n";
		std::system("pause");
	}

	~InBufferFail() throw () {}
};

struct OutBufferFail : public exception {
	string error;

	OutBufferFail(string e): error(e) {};

	void what() throw () {
		cout << "Writing fail: "<< error << ".\n";
		std::system("pause");
	}

	~OutBufferFail() throw () {}
};

