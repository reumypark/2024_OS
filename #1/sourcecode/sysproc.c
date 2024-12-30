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
#ifndef PROC_H
#define PROC_H

//define SEEK_SET, SEEK_CUR, SEEK_END
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct file;

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


//adding lseek system call
int
sys_lseek(void)
{
  //SEEK_SET(from the biginning), SEEK_CUR(from the current), SEEK_END(from the end)
  
  //fd(file descriptor), offset, whence is paramter of lseek function
    int fd = -1, offset = 0, whence = 0;
    struct file *f = 0;

    //Using argin() to deliver integrers.
    if (argint(0, &fd) < 0 || argint(1, &offset) < 0 || argint(2, &whence) < 0) {
        return -1;  // If cannot retrieve the arguments, return an error
    }

    //proc declaration. And using myproc(), so get current process.
    struct proc *p = myproc();
    //check file descriptor is valid or not.
    if (fd < 0 || fd >= NOFILE || (f = p->ofile[fd]) == 0) {
        cprintf("Error: invalid file descriptor\n");
        return -1;
    }

    if (f->type != FD_INODE) {
        return -1;
    }

    // calculate the new offset
    int new_offset;
    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = f->off + offset;
            break;
        case SEEK_END:
            new_offset = f->ip->size + offset;
            break;
        default:
            return -1;
    }

    // failed case
    if (new_offset < 0) {
        return -1;
    }
    //update existing offset
    f->off = new_offset;

    //return new offset
    return new_offset;
}
#endif
