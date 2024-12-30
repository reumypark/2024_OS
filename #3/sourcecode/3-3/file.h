#include <stddef.h>
struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};

#define MAX_NODES 100
#define RED 0
#define BLACK 1
struct rbnode {
  //entry time provide insert or hit time(bigger means latest approach)
  int entry_time;
  int key;
  int val;
  int color; // 0 -> red, 1 -> black
  int parent_key;

  struct rbnode *parent;
  struct rbnode *right;
  struct rbnode *left;
};

struct rbtree {
  struct rbnode *root;
  struct rbnode *NIL;
  int node_count;
  struct rbnode nodes[MAX_NODES+1];
};

void initialize_rb_tree(struct inode *ip);
struct rbnode *add_node(struct rbtree *tree, int key, int value, int tick_value);

// in-memory copy of an inode
struct inode {
  struct rbtree *tree;

  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+NINDIRECT1+NINDIRECT2+NINDIRECT3];
};

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
