#pragma once

#include <iostream>
#include <string>
using namespace std;

//Main.cpp
struct OutOfBoundExc : public exception {
	const char * what() const throw() {
		cout << "Out of bound\n";
	}

	~OutOfBoundExc() throw() {}
};

struct ArchAndDearchExc : public exception {
	const char * what() const throw() {
		cout << "Can't archive and dearchive files at the same time\n";
	}
};

//gzip.cpp

struct InputOpenFail : public exception {
	string filename;

	InputOpenFail(string f): filename(f) {};

	const char * what() const throw() {
		cout << "File \"" << filename << "\" could not be opened for reading\n";
		std::system("pause");
	}

	~InputOpenFail() throw() {}
};

struct WrongSizeExc : public exception {
	int origSz; 
	int curSz;

	WrongSizeExc(int o, int c): origSz(o), curSz(c) {};

	const char * what() const throw() {
		cout << "Original size " << origSz << " does not match with current size " << curSz << ".\n";
		std::system("pause");
	}

	~WrongSizeExc() throw() {}
};

struct WrongCRCExc : public exception {
	int origCRC;
	int curCRC;

	WrongCRCExc(int o, int c): origCRC(o), curCRC(c) {};

	const char * what() const throw() {
		cout << "Original CRC32 " << origCRC << " does not match with current CRC32 " << curCRC << ".\n";
		std::system("pause");
	}

	~WrongCRCExc() throw() {}
};

//Inflate.cpp
struct InflateDecodeFail : public exception {
	string error;

	InflateDecodeFail(string e): error(e) {};

	const char * what() const throw() {
		cout << "Failed to decode current file: " << error << ".\n";
		std::system("pause");
	}

	~InflateDecodeFail() throw() {}
};

//Trie.cpp
struct WrongValExc : public exception {
	int val;

	WrongValExc(int v): val(v) {};

	const char * what() const throw() {
		cout << "Wrong literal value " << val << ".\n";
		std::system("pause");
	}

	~WrongValExc() throw() {}
};

struct WrongCodeExc : public exception {
	int code;

	WrongCodeExc(int c): code(c) {};

	const char * what() const throw() {
		cout << "Code " << code << " doesn't exist in the tree\n";
		std::system("pause");
	}

	~WrongCodeExc() throw() {}
};

//Buffer.cpp
struct InBufferFail : public exception {
	string error;

	InBufferFail(string e): error(e) {};

	const char * what() const throw() {
		cout << "Reading fail: "<< error << ".\n";
		std::system("pause");
	}

	~InBufferFail() throw() {}
};

struct OutBufferFail : public exception {
	string error;

	OutBufferFail(string e): error(e) {};

	const char * what() const throw() {
		cout << "Writing fail: "<< error << ".\n";
		std::system("pause");
	}

	~OutBufferFail() throw() {}
};

