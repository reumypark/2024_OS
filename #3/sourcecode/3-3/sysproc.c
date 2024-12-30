#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "defs.h"
#include "file.h"
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_lseek(void)
{
  struct file *f = 0;
  int fd, n, whence;
  uint off;
  
  if (argfd(0, &fd, &f) < 0 || argint(1, &n) < 0 || argint(2, &whence) < 0) return -1;
  if (f->type != FD_INODE) return -1;

  switch (whence){ 
    case SEEK_SET: 
      off = 0;
      break;
    case SEEK_CUR:
      off = f->off;
      break;
    case SEEK_END:
      off = f->ip->size; 
      break;
    default: 
      return -1; 
      break;
  }
  
  off += n;
  if(off < 0 || off > f->ip->size) 
    return -1;
  
  f->off = off;
  return off;
}

void in_order_print(struct rbtree *tree, struct rbnode *node, int fd, int depth, int parent_key) {
  if(node == tree->NIL) return;

  if(node->color == RED) cprintf("key: %d, value: %d, depth: %d, color: R, parent key: %d\n", node->key, node->val, depth, parent_key);
  else cprintf("key: %d, value: %d, depth: %d, color: B, parent key: %d\n", node->key, node->val, depth, parent_key);
  
  //left subtree
  in_order_print(tree, node->left, fd, depth+1, node->key);
  
  //right subtree
  in_order_print(tree, node->right, fd, depth+1, node->key);
}

int
sys_rb_print(void)
{ 
  int fd;
  if(argint(0, &fd) < 0) return -1;
  
  struct file *f = myproc()->ofile[fd];
  if(!f || !f->ip) {
    cprintf("Invalid file descriptor or inode!\n");
    return 0;
  }

  struct inode *ip = f->ip;
  if (!ip->tree) {
    return 0;
  }
  struct rbtree *tree = ip->tree;
  if(tree->root == tree->NIL) {
   return 0;
  }

  in_order_print(tree, tree->root, fd, 1, -1);
  
  return 0;
}

int
sys_rb_count(void)
{
  int fd;
  if (argint(0, &fd) < 0) return -1;
  
  cprintf("bmap access count: %d, cache hit count: %d, disk access count: %d\n", bmap_count, cache_hit_count, disk_access_count);

  return 0;
}
