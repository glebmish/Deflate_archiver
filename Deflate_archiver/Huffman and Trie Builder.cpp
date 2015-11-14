#include <algorithm>

#include "Huffman and Trie Builder.h"

#include "Exceptions.h"
#include "MacrosAndPrecomputers.h"

void Huffman_decoder(const vector<int> &lens, vector<int> &codes, vector<int> &lenCount) {
	if (lenCount[0] == lens.size()) return;
	codes.resize(lens.size());

	int code = 0;
	lenCount[0] = 0;
	vector<int> nextCode(lenCount.size());	
	for (uint bits = 1; bits < lenCount.size(); bits++) {
		code = (code + lenCount[bits - 1]) << 1;
		nextCode[bits] = code;
	}

	for (uint n = 0; n < lens.size(); n++) {
		int len = lens[n];
		if (len != 0) {
			codes[n] = nextCode[len];
			nextCode[len]++;
		}
	}
}

struct Node {
	int lit;
	double prob;
	Node* left;
	Node* right;

	Node(int l, double p, Node* ll, Node* rr): lit(l), prob(p), left(ll), right(rr) {}
	~Node() {
		delete left;
		delete right;
	}
};

bool comp(Node *a, Node *b) {
	return a->prob > b->prob;
}

void update(Node *node, vector<int> &len) {
	if (node->lit != -1) { 
		len[node->lit]++;
	} else {
		update(node->left, len);
		update(node->right, len);
	}
}

void Huffman_builder(vector<int> &prob, vector<int> &len) {
	vector<Node*> tree;
	for (int i = 0; i < prob.size(); i++)
		if (prob[i] > 0)
			tree.push_back(new Node(i, prob[i], NULL, NULL));
	sort(tree.begin(), tree.end(), comp);

	while (tree.size() > 1) {
		int last = tree.size() - 1;
		
		Node *anc = new Node(-1, tree[last]->prob + tree[last - 1]->prob, tree[last - 1], tree[last]);
		tree.pop_back();
		tree.pop_back();

		update(anc, len);

		vector<Node*>::iterator pos = upper_bound(tree.begin(), tree.end(), anc, comp);
		tree.insert(pos, 1, anc);
	}
	if (tree.size() > 0)
		delete tree[0];
	for (int i = 0; i < len.size(); i++)
		if (len[i] > 15)
			len[i] = 15;
}		

Trie build_trie(InBuffer &buf, const vector<int> &code, const vector<int> &len) {
	try {
		Trie trie(buf);
		for (uint i = 0; i < code.size(); ++i) {
			if (len[i] != 0) {
				trie.add_word(code[i], len[i], i);
			}
		}
		return trie;
	}
	catch (WrongValExc e) {
		e.what();
		string error = "failed building tree";
		throw InflateDecodeFail(error);
	}
}