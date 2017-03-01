#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdlib>
using namespace std;

#include "gzip.h"
#include "Exceptions.h"

string get_path(string &filename) {
	int delim = filename.find_last_of('/');
	string ext;
	return ext.assign(filename, 0, delim + 1);
}

string get_ext(string &filename) {
	int delim = filename.find_last_of('.');
	string ext;
	return ext.assign(filename, delim + 1, string::npos);
}

string get_name(string &filename) {
	int delim = filename.find_last_of("/");
	string name;
	return name.assign(filename, delim + 1, string::npos);
}

string get_parent_folder(string &filename) {
	int endDelim = filename.find_last_of("/");
	int stDelim = filename.find_first_of("/", endDelim - 1);
	string parentFolder;
	return parentFolder.assign(filename, stDelim + 1, endDelim - 1);
}

string next_arg(int argc, char* argv[], int i) {
	if (i >= argc)
		throw OutOfBoundExc();
	else return((string)argv[i]);
}

/*flags
 * -a, --archive    = archive in extension comes as 1 argument after this flag (without point)
                      archive n files comes as n arguments after the extension (n > 0)
					  if 1st char of argument k is '-' then k-1 filenames and argument k recognizes as flag
 * -n, --name       = if -a was set it is name of output archive
                      if not set arhive name is name of the file (if only 1 file)
					  or folder name of the 1st file (if >1 files)
 * -d, --dearchive  = dearchive 1 file comes as 1 argument after this flag
 * -p, --place      = place result of programm in folder comes as 1 argument after this flag
 *                    if place is not defined by user then place is the folder of first input file
 * all other arguments are ignored
 */

struct {
	unsigned archive   : 1;
	unsigned name      : 1;
	unsigned dearchive : 1;
	unsigned place     : 1;
} flags;

int main(int argc, char* argv[]) {
#ifdef DEBUG
	debug();
#else
	try {
		vector<string> filename;
		string outputFolder = "";
		string possibleOutputFolder = "";
		string ext;
		string archiveName;
		memset(&flags, 0, sizeof(flags));
		int i = 1;
		while (i < argc) {
			string arg = next_arg(argc, argv, i++);
			if (arg == "-a" || arg == "--archive") {
				flags.archive = 1;
				if (flags.dearchive) throw ArchAndDearchExc();

				ext = next_arg(argc, argv, i++);
				do {
					filename.push_back(next_arg(argc, argv, i++));
					if (possibleOutputFolder == "")
						possibleOutputFolder = get_path(filename.back());
				} while (filename.back()[0] != '-' && i < argc);
				if (filename.back()[0] == '-') {
					i--;
					filename.pop_back();
				}

				for (int i = 0; i < filename.size(); ++i)
					cout << filename[i] << endl;

			} else if (arg == "-d" || arg == "--dearchive") {
				flags.dearchive = 1;
				if (flags.archive) throw ArchAndDearchExc();

				filename.push_back(next_arg(argc, argv, i++));
				ext = get_ext(filename.back());
				if (possibleOutputFolder == "")
					possibleOutputFolder = get_path(filename.back());

			} else if (arg == "-p" || arg == "--place") {
				flags.place = 1;
				outputFolder = next_arg(argc, argv, i++);

			} else if (arg == "-n" || arg == "--name") {
				flags.name = 1;
				archiveName = next_arg(argc, argv, i++);
			} else continue;
		}

		if (!flags.place)
			outputFolder = possibleOutputFolder;
					
		if (flags.archive) {
			if (!flags.name) {
				if (filename.size() == 1) 
					archiveName = get_name(filename[0]) + "." + ext;
				else 
					archiveName = get_parent_folder(filename[0]) + "." + ext;
			}
			if (ext == "gz")
				gzip_archive(filename, archiveName, outputFolder);
			else {
				cout << "Unknown extension\n";
				return 0;
			}
		} else if (flags.dearchive) {
			if (ext == "gz") {
				if (filename.size() == 0) {
					cout << "There is no file to dearchive\n";
					return 0;
				}
				gzip_dearchive(filename[0], outputFolder);
			} else {
				cout << "Unknown extension\n";
				return 0;
			}
		} else {
			cout << "No work to do\n";
			return 0;
		}
	}
	catch (ArchAndDearchExc e) {
		std::cout << e.what();
	}
	return 0;
#endif
}