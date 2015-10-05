#pragma once

#include <iostream>
#include <string>
using namespace std;

//Main.cpp
struct OutOfBoundExc : public exception {
	void what() {
		cout << "Out of bound\n";
	}
};

struct ArchAndDearchExc : public exception {
	void what() {
		cout << "Can't archive and dearchive files at the same time\n";
	}
};

//gzip.cpp

struct InputOpenFail : public exception {
	string filename;

	InputOpenFail(string f): filename(f) {};

	void what() {
		cout << "File \"" << filename << "\" could not be opened for reading\n";
		std::system("pause");
	}
};

struct WrongSizeExc {
	int origSz;
	int curSz;

	WrongSizeExc(int o, int c): origSz(o), curSz(c) {};

	void what() {
		cout << "Original size " << origSz << " does not match with current size " << curSz << ".\n";
		std::system("pause");
	}
};

struct WrongCRCExc {
	int origCRC;
	int curCRC;

	WrongCRCExc(int o, int c): origCRC(o), curCRC(c) {};

	void what() {
		cout << "Original CRC32 " << origCRC << " does not match with current CRC32 " << curCRC << ".\n";
		std::system("pause");
	}
};

//Inflate.cpp
struct InflateDecodeFail : public exception {
	string error;

	InflateDecodeFail(string e): error(e) {};

	void what() {
		cout << "Failed to decode current file: " << error << ".\n";
		std::system("pause");
	}
};

//Trie.cpp
struct WrongValExc : public exception {
	int val;

	WrongValExc(int v): val(v) {};

	void what() {
		cout << "Wrong literal value " << val << ".\n";
		std::system("pause");
	}
};

struct WrongCodeExc : public exception {
	int code;

	WrongCodeExc(int c): code(c) {};

	void what() {
		cout << "Code " << code << " doesn't exist in the tree\n";
		std::system("pause");
	}
};

//Buffer.cpp
struct InBufferFail : public exception {
	string error;

	InBufferFail(string e): error(e) {};

	void what() {
		cout << "Reading fail: "<< error << ".\n";
		std::system("pause");
	}
};

struct OutBufferFail : public exception {
	string error;

	OutBufferFail(string e): error(e) {};

	void what() {
		cout << "Writing fail: "<< error << ".\n";
		std::system("pause");
	}
};

