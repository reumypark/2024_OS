#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

int bmap_count = 0;
int cache_hit_count = 0;
int disk_access_count = 0;
//save delete node's arrray index and reuse that array index.
int del_index = 0;
//it's for detecting entry time.
int global_entry_time = 0;

void insert_fixup(struct rbtree *tree, struct rbnode *node);
void left_rotate(struct rbtree *tree, struct rbnode *x);
void right_rotate(struct rbtree *tree, struct rbnode *y);
void erase_oldest_node(struct rbtree *tree);
int remove_node(struct rbtree *tree, struct rbnode *node);
void transplant(struct rbtree *tree, struct rbnode *u, struct rbnode *v);
struct rbnode *minimum(struct rbtree *tree, struct rbnode *node);
void fix_remove(struct rbtree *tree, struct rbnode *node);
void print_tree(struct rbnode *root);

//This function for debugging. Print_tree can show all tree structure.
void print_tree(struct rbnode *root) {
	cprintf("key:color - %d:%d, left key: %d, right key: %d\n", root->key, root->color, root->left->key, root->right->key);
	if (root->left->key != 0)
	print_tree(root->left);
	if (root->right->key != 0)
	print_tree(root->right);
}

//add the node in tree
//if tree's node over the MAX_NODES(100) then erase and add the node
struct rbnode *add_node(struct rbtree *tree, int key, int value, int tick_value) {
  if (tree->node_count >= MAX_NODES) {
    erase_oldest_node(tree);
  }

  struct rbnode *new_node;

  if(del_index >= 0) {
    new_node = &tree->nodes[del_index]; 
    tree->node_count++;
    del_index = -1;
  }
  else new_node = &tree->nodes[tree->node_count++];

  if (new_node == NULL) return NULL;

  new_node->entry_time = tick_value;
  new_node->key = key;
  new_node->val = value;
  new_node->color = RED;
  new_node->parent = tree->NIL;
  new_node->left = tree->NIL;
  new_node->right = tree->NIL;

  if(tree->root == tree->NIL) {
    tree->root = new_node;
    tree->root->color = BLACK;
    return new_node;
  }

  struct rbnode *curr = tree->root;
  struct rbnode *prev = tree->NIL;

  while(curr != tree->NIL) {
    prev = curr;
    if (key < curr->key) curr = curr->left;
    else curr = curr->right;
  }

  if(new_node->key < prev->key) prev->left = new_node;
  else prev->right = new_node;
  new_node->parent = prev;
  
  //fix the rbtree
  insert_fixup(tree, new_node);
  return new_node;
}

//initialize the rb tree
void initialize_rb_tree(struct inode *ip) {
    ip->tree = (struct rbtree *)kalloc();
    if (!ip->tree) {
      panic("fail");
    }

    ip->tree->NIL = (struct rbnode *) &ip->tree->NIL;
    if(!ip->tree->NIL) {
        panic("Failed to allocate NIL node");
    }

    ip->tree->NIL->key = 0;
    ip->tree->NIL->val = 0;
    ip->tree->NIL->color = BLACK;
    ip->tree->NIL->entry_time = 0;
    ip->tree->NIL->parent = NULL;
    ip->tree->NIL->left = NULL;
    ip->tree->NIL->right = NULL;

    ip->tree->root = ip->tree->NIL;
    ip->tree->node_count = 0;
}

//After insert, fix rbtree to rule.
void insert_fixup(struct rbtree *tree, struct rbnode *node) {
  while (node != tree->root && node->parent->color == RED) {
    if(node->parent == node->parent->parent->left) {
      //case 1
      struct rbnode *uncle = node->parent->parent->right;
      // uncle is red
      if(uncle != NULL && uncle->color == RED) {	//case 1-1 : recoloring
        node->parent->color = BLACK;
        uncle->color = BLACK;
        node->parent->parent->color = RED;
        node = node->parent->parent;
      } else { 
        if(node == node->parent->right) {	//case 1-2 : left rotate
          node = node->parent;
          left_rotate(tree, node);
        }
        else {		//case 1-3 : right rotate
          node->parent->color = BLACK;
          node->parent->parent->color = RED;
          right_rotate(tree, node->parent->parent);
        }
      }
    }
    //case 2 : that parent node is grandparent's node's right child
    else {
      struct rbnode *uncle = node->parent->parent->left;	
      //uncle is red
      if(uncle != NULL && uncle->color == RED) {	//case 2-1 : recoloring
        node->parent->color = BLACK;
        uncle->color = BLACK;
        node->parent->parent->color = RED;
        node = node->parent->parent;
      } else {
        if(node == node->parent->left) {	//case 2-2 : right rotate
          node = node->parent;
          right_rotate(tree, node);
        }
        else {		//case 2-3 : left rotate
          node->parent->color = BLACK;
          node->parent->parent->color = RED;
          left_rotate(tree, node->parent->parent);
        }
      }
    }
  }
  tree->root->color = BLACK;
}

//right rotate rb tree
void right_rotate(struct rbtree *tree, struct rbnode *y) {
  struct rbnode *x = y->left;
  y->left = x->right;
  if(x->right != tree->NIL) {
    x->right->parent = y;
  }
  
  x->parent = y->parent;

  if(y->parent == tree->NIL) {
    tree->root = x;
  } else if (y == y->parent->right) {
    y->parent->right = x;
  } else {
    y->parent->left = x;
  }

  x->right = y;
  y->parent = x;
}

//left rotate rb tree
void left_rotate(struct rbtree *tree, struct rbnode *x) {
  struct rbnode *y = x->right;
  x->right = y->left;

  if(y->left != tree->NIL) {
    y->left->parent = x;
  }
  y->parent = x->parent;
  
  if (x->parent == tree->NIL) {
    tree->root = y;
  } else if (x == x->parent->left) {
    x->parent->left = y;
  } else {
    x->parent->right = y;
  }

  y->left = x;
  x->parent = y;
}

//If you want to erase node, have to reset that node
void reset_node(struct rbtree *tree, struct rbnode *node) {
    node->entry_time = 0;
    node->key = 0;
    node->val = 0;
    node->color = BLACK;
    node->entry_time = 0;
    node->parent = tree->NIL;
    node->left = tree->NIL;
    node->right = tree->NIL;
}

//return : oldest node struct
struct rbnode* find_oldest_node_recursive(struct rbnode* current, struct rbnode* NIL, struct rbnode* oldest) {
    if (current == NIL) return oldest;

    //Check smaller entry_time
    if (current->entry_time < oldest->entry_time) {
        oldest = current;
    }

    //Recursively tour left and right subtree
    oldest = find_oldest_node_recursive(current->left, NIL, oldest);
    oldest = find_oldest_node_recursive(current->right, NIL, oldest);

    return oldest;
}

//Find oldest node and erase that node
void erase_oldest_node(struct rbtree *tree) {
  if (tree->node_count <= 0) return;

  struct rbnode *oldest = tree->root;

  oldest = find_oldest_node_recursive(tree->root, tree->NIL, oldest);  
 
  for(int i = 0; i < 100 ; i++){
    struct rbnode *check = &tree->nodes[i];
    if(check->key == oldest->key) {
      del_index = i;
      break;
    }
  }
  
  remove_node(tree, oldest);

}

//remove the node from tree
int remove_node(struct rbtree *tree, struct rbnode *node) {
  struct rbnode *del = node;
  struct rbnode *base;
  int original_color = node->color;

  //case 1: Node don't have left child
  if(node->left == tree->NIL) {
    base = node->right;
    transplant(tree, node, node->right);
  } 
  else if (node->right == tree->NIL) {	//case 2: Node don't have right child
    base = node->left;
    transplant(tree, node, node->left);
  } 

  else {	//case 3: Node have two children
    del = minimum(tree, node->right);
    original_color = del->color;
    base = del->right;

    if(del->parent == node) {
      base->parent = del;
    }
    else {
      transplant(tree, del, del->right);
      del->right = node->right;
      del->right->parent = del;
    }

    transplant(tree, node, del);
    del->left = node->left;
    del->left->parent = del;
    del->color = node->color;
  }
 
  // if removed node's color is black, fix rbtree to rule.
  if(original_color == BLACK) {
    fix_remove(tree, base);
  }

  //renew node_count num and reset target node
  if(del_index >= 0) reset_node(tree, &tree->nodes[del_index]);
  tree->node_count--; 

  return 0;
} 

//Raplace subtree with another tree
void transplant(struct rbtree *tree, struct rbnode *u, struct rbnode *v) {
  if(u->parent == tree->NIL) tree->root = v;
  else if(u == u->parent->left) u->parent->left = v;
  else u->parent->right = v;

  v->parent = u->parent;
  return;
}

//Find minimum node in tree
struct rbnode *minimum(struct rbtree *tree, struct rbnode *node) {
  struct rbnode *r = node;
  if(r == tree->NIL) return r;

  while(r->left != tree->NIL) r = r->left;
  return r;
}

//After remove the node, fix the rb tree to rule.
void fix_remove(struct rbtree *tree, struct rbnode *node) {
  struct rbnode *sibling;
  while ((node != tree->root) && (node->color == BLACK)) {
    if (node == node->parent->left) {
      sibling = node->parent->right;
      //case 1 : Sibling is red
      if(sibling->color == RED) {
        sibling->color = BLACK;
        node->parent->color = RED;
        left_rotate(tree, node->parent);
        sibling = node->parent->right;
      }

      //case 2 : Sibling and sibling's children are black
      if (sibling->color == BLACK && sibling->left->color == BLACK && sibling->right->color == BLACK) {
	sibling->color = RED;
        node = node->parent;
      }

      else {
        //case 3 : Sibling's right child is black, left child is red
        if(sibling->right->color == BLACK) {
          sibling->left->color = BLACK;
          sibling->color = RED;
          right_rotate(tree, sibling);
          sibling = node->parent->right;
        }

        //case 4 : Sibling's right child is red.
        sibling->color = node->parent->color;
        node->parent->color = BLACK;
        sibling->right->color = BLACK;
        left_rotate(tree, node->parent);
        node = tree->root;
      }
    } 
    
    else {
      //The algorithm is the same as (node == node->parent->left). just change left right direction and red black color.
      sibling = node->parent->left;
      //case 1 : Sibling is red
      if(sibling->color == RED) {
        sibling->color = BLACK;
        node->parent->color = RED;
        right_rotate(tree, node->parent);
        sibling = node->parent->left;
      }

      //case 2 : Sibling and sibling's children are black
      if(sibling->color == BLACK && sibling->right->color == BLACK && sibling->left->color == BLACK) {
        sibling->color = RED;
        node = node->parent;
      } 
      else {
        //case 3 : Sibling's right child is black, left child is red
        if(sibling->left->color == BLACK) {
          sibling->right->color = BLACK;
          sibling->color = RED;
          left_rotate(tree, sibling);
          sibling = node->parent->left;
        }
        //case 4 : Sibling's right child is red.
        sibling->color = node->parent->color;
        node->parent->color = BLACK;
        sibling->left->color = BLACK;
        right_rotate(tree, node->parent);
        node = tree->root;
      }
    }
  }
  node->color = BLACK;
}

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE)
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
  if(f->type == FD_INODE){
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}

// Read from file f.
int
fileread(struct file *f, char *addr, int n)
{
  int r;
  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE)
    return piperead(f->pipe, addr, n);
  
  if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, addr, f->off, n)) > 0) {
     f->off += BSIZE;
    }
    iunlock(f->ip);
    return r;
  }
  panic("fileread");
}

//PAGEBREAK!
// Write to file f.
int
filewrite(struct file *f, char *addr, int n)
{
  int r;

  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return pipewrite(f->pipe, addr, n);
  if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    return i == n ? n : -1;
  }
  panic("filewrite");
}

