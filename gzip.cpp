#include "gzip.h"

#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>

#include "./deflate/Inflate.h"
#include "./deflate/Deflate.h"
#include "./exceptions/Exceptions.h"
#include "./util/CRC32.h"

typedef unsigned char uchar;
typedef unsigned int uint;

union flags {
	char a;
	struct {
		unsigned FTEXT : 1;
		unsigned FHCRC : 1;
		unsigned FEXTRA : 1;
		unsigned FNAME : 1;
		unsigned FCOMMENT : 1;
	} F;
};

void gzip_archive(vector<string> &filename, string &archiveName, string &outputFolder) {
	fstream out((outputFolder + archiveName).c_str(), ios_base::out | ios_base::binary);
	
	for (int i = 0; i < filename.size(); i++) {
		try {
			fstream in(filename[i].c_str(), ios_base::in | ios_base::binary);
			if (in.fail())
				throw InputOpenFail(filename[i]);

			uchar ID1 = 31;      //fixed = 31
			uchar ID2 = 139;     //fixed = 139
			uchar CM = 8;        //0-7 reserved, 8 = deflate
			flags FLG;
			FLG.a = 0;
			FLG.F.FNAME = 1;     //filename included
			int MTIME = time(NULL);  //unix-time of last modification of the original file
			uchar XFL = 2;       //Extra flags
						         //if CM = 8 -> = 2 - max compression, = 4 -> max speed
			uchar OS = 0;	     //operating system on which compression took place (look in cpecs)

			out.put(ID1);
			out.put(ID2);
			out.put(CM);
			out.put(FLG.a);
			out.write((char*)&MTIME, 4);
			out.put(XFL);
			out.put(OS);

			unsigned ISIZE;
			unsigned CRC32 = 0;
			in.seekg(0, in.end);
			ISIZE = in.tellg();
			in.seekg(0, in.beg);
			CRC32 = crc32(in, ISIZE);
			in.seekg(0, in.beg);

			string fname;
			fname.assign(filename[i], filename[i].find_first_of("/") + 1, string::npos);
			out.write(fname.c_str(), fname.size());
			out.put(0);
			
			deflate(in, out);

			out.write((char*)&CRC32, 4);
			out.write((char*)&ISIZE, 4);
		}
		catch (InputOpenFail e) {
			e.what();
		}
	}
}

void gzip_dearchive(string &filename, string &outputFolder) {
	int ct = 0;
	fstream in(filename.c_str(), ios_base::in | ios_base::binary);
	if (in.fail())
		throw InputOpenFail(filename);

	while (in.get() != EOF && ++ct) {
		try {
			in.unget();
			uchar ID1;  //fixed = 31
			uchar ID2;  //fixed = 139
			uchar CM;   //0-7 reserved, 8 = deflate
			flags FLG;  //bit 0   FTEXT
						//bit 1   FHCRC
						//bit 2   FEXTRA
						//bit 3   FNAME
						//bit 4   FCOMMENT
						//bit 5-7 reserved
			int MTIME;  //unix-time of last modification of the original file
						//IF NOT -> time at which compression started
						//= 0 -> no time stamp is available
			uchar XFL;  //Extra flags
						//if CM = 8 -> = 2 - max compression, = 4 -> max speed
			uchar OS;	//operating system on which compression took place (look in cpecs)

			ID1 = in.get();
			ID2 = in.get();
			CM = in.get();
			FLG.a = in.get();
			in.read((char*)&MTIME, sizeof(MTIME));
			XFL = in.get();
			OS =in.get();
			if (ID1 != 31 || ID2 != 139)
				cout << "error"; //error

			string filename; //original file name, zero-terminated (if FNAME is set)
			string comment; //file comment, zero-terminated (if FCOMMENT is set)
			short CRC16; //CRC16 for gzip header (if FHCRC is set)

			if (FLG.F.FEXTRA) {
				//extra field (if FEXTRA is set), didn't stored
				short XLEN; //length of extra field
				in >> XLEN;
				for (int i = 0; i < XLEN; ++i) {
					char c;
					in >> c;
				}
			}

			if (FLG.F.FNAME) {
			char c;
				do {
					in.get(c);
					filename += c;
				} while(c != '\0');
				filename = outputFolder + filename;
			} else {
				char tmp[20];
				sprintf(tmp, "unnamed file %d", ct);
				filename = tmp;
			}

			if (FLG.F.FCOMMENT) {
				char c;
				do {
					in.get(c);
					comment += c;
				} while(c != '\0');
			}

			if (FLG.F.FHCRC) {
				in >> CRC16;
			}

			fstream out(filename.c_str(), ios_base::out | ios_base::binary);
			while (out.fail()) {
				out.clear();
				int delim = filename.find_last_of('.');
				filename.insert(delim, " - copy");
				out.open(filename.c_str(), ios_base::out | ios_base::binary);
			}

			//compressed blocks
			if (CM == 8) {
				try {
					inflate(in, out);
				}
				catch (InflateDecodeFail e) {
					out.close();
					remove(filename.c_str());
					throw e;
				}
			}

			unsigned CRC32; //CRC32 value of the uncompressed data
			unsigned ISIZE; //the size of the original (uncompressed) input data modulo 2^32
			int size = out.tellp(); //size of the decoded file

			in.read((char*)&CRC32, sizeof(CRC32));
			in.read((char*)&ISIZE, sizeof(ISIZE));
			if (ISIZE != size) throw WrongSizeExc(ISIZE, size);
			
			out.close();
			fstream in(filename.c_str(), ios_base::in | ios_base::binary);
			unsigned realCRC32 = crc32(in, size);
			if (CRC32 != realCRC32) throw WrongCRCExc(CRC32, realCRC32);
		}
		catch (InputOpenFail e) {
			e.what();
		}
		catch (WrongSizeExc e) {
			e.what();
		}
		catch (WrongCRCExc e) {
			e.what();
		}
		catch (InflateDecodeFail e) {
			e.what();
		}
	}
}