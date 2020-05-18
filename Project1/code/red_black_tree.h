#ifndef __RBT__
#define __RBT__

#include "voter.h"
#include "bloom_filter.h"

typedef struct BF *BFPtr;

#define BLACK false
#define RED true

using namespace std;

typedef struct node *NodePtr;

struct node {

	bool colour;                                    // 0 for BLACK, 1 for RED
	voter* info;
	bool hasvoted;
	NodePtr predecessor;
	NodePtr left, right;

	node(bool c = BLACK, voter* v = NULL, bool hv = 0, NodePtr p = NULL, NodePtr l = NULL, NodePtr r = NULL): colour(c), info(v), hasvoted(hv), predecessor(p), left(l), right(r)
	{ /* std::cout << "A new node has been created!\n"; */ }

	~node(void)
	{ delete info; delete left; delete right; /* std::cout << "A node is being destructed\n"; */ }
};


typedef struct RBT *RBTPtr;

struct RBT {
	NodePtr root;
	unsigned int size;

	RBT(void);
	~RBT(void);

	bool search(char* id);
	voter* get_voter(char* id);
	bool has_voted(char* id);
	voter* vote(char* id);

	void left_rotation(NodePtr x);
	void right_rotation(NodePtr y);

	void insert(voter* v);
	void fix_insertion(NodePtr z);

	void transplant(NodePtr u, NodePtr v);
	NodePtr min_of_subTree(NodePtr x);

	bool remove(char* id);
	void fix_deletion(NodePtr x);

	void recreate_bloom_filter(BFPtr bf);
	void Inorder_Recreation(BFPtr bf, NodePtr temp);

	void print(FILE* output);
	void recursive_print(NodePtr n, FILE* output);

	void begin_count(void);
	void count_black_nodes(NodePtr x, int count);
};



#endif
