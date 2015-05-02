// The functions described here are very low level voxeltree related functions
// they should be inlined in the implementations
// they are defined to simplify the interface needed by vox.c and hence voxtree uses an opaque pointer

//requires vox.h


// returns the next node in the tree relative to memory, might up or down the tree graph.
voxtree * tree_next(voxtree * vt);
u8 * tree_color(voxtree * vt);
bool tree_is_node(voxtree * vt);
bool tree_is_leaf(voxtree * vt);

// Skips a subtree.
voxtree * tree_skip(voxtree * vt);
// Creates a new voxel context
voxtree_ctx * voxtree_ctx_make(size_t capacity);

// Returns the capacity of the voxtree context.
size_t voxtree_ctx_capacity(voxtree_ctx * ctx);

// Returns the size of the voxtree.
size_t voxtree_ctx_size(voxtree_ctx * ctx);

// Creates a new voxel context from a pointer.
// can be dangerous to use.
voxtree_ctx * voxtree_ctx_make_from_ptr(voxtree * tree, size_t size);

// Gets the tree of a voxel context. This pointer is subject to change.
// Functions that take ctx as input might make the pointer returned by 
// this invalid. All these functions also returns a pointer.
voxtree * voxtree_ctx_tree(voxtree_ctx * ctx);

// Converts a leaf to a node. new leaves will have same color as vt.
voxtree * tree_leaf_to_node(voxtree * vt, voxtree_ctx * ctx);

// Converts a node to a leaf. the node will not have the color set.
voxtree * tree_node_to_leaf(voxtree * vt, voxtree_ctx * ctx);

// Inserts a whole tree into the node. Cannot be from the tree itself.
voxtree * tree_insert_node(voxtree * node, voxtree * node_data, voxtree_ctx * ctx);

// test
// loads a simple tree with the size of 17 cells
voxtree * tree_make_test();
