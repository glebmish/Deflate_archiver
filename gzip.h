#pragma once

#include <fstream>
#include <vector>
using namespace std;

void gzip_archive(vector<string> &filename, string &archiveName, string &outputFolder);
void gzip_dearchive(string &filename, string &outputFolder);