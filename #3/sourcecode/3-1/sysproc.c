#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

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

//for Assignment3
//
int
sys_ssusbrk(void)
{
  int page_size, tick_size;
  struct rtcdate r;
  struct proc *proc = myproc();
  if(argint(0, &page_size) < 0 || argint(1, &tick_size) < 0) {
    return -1; //return error
  }
  
  if(page_size > 0) {	//memory allocation
    if(page_size % PGSIZE != 0) return -1; 
    uint oldsz = proc->sz;
    uint newsz = proc->sz + (page_size);

    //allocate virtural memory
    if(mappages(proc->pgdir, (void*)oldsz, page_size, 0, ((PTE_U | PTE_W)) < 0)) return -1;	//page mapping fault then return -1

    proc->sz = newsz;
    proc->vp += page_size/PGSIZE;	//update virtual page count
    return oldsz;
    
  } else if(page_size < 0) {	//memory deallocation
    if((-1 * page_size) % PGSIZE != 0) return -1;
    if(tick_size <= 0) return -1; 
	  
    cmostime(&r);
    cprintf("Memory deallocation request(%d): %d-%d-%d %d:%d:%d\n", tick_size, r.year, r.month, r.day, r.hour, r.minute, r.second);
    
    //Case 1 : release is in effect -> add the page size
    if(proc->release_time > 0) {
      proc->pending_release += page_size; 
    } else {	//Case 2 : release is not in effect -> just change pending_release to page_size
      proc->pending_release = page_size;
    }
    
    //update release time
    proc->release_time = ticks + tick_size; 

    return proc->sz;
  }
  
  return -1;
}

// print information about the currently allocated virtual and physical memory
// memstat function lacate in proc.c
int
sys_memstat(void)
{
  return memstat();

}
