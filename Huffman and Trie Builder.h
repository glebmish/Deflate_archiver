#pragma once

#include <vector>
using namespace std;

#include "Trie.h"
#include "Buffer.h"

struct Node;

void Huffman_decoder(const vector<int> &lens, vector<int> &codes, vector<int> &lenCount);
void Huffman_builder(vector<int> &prob, vector<int> &len);
Trie build_trie(InBuffer &buf, const vector<int> &code, const vector<int> &len);