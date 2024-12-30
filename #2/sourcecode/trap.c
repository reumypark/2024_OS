#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }

    // Update process tatistics in every tick
    if (myproc()) {
	if (myproc()->state == RUNNING) {
	    myproc()->cpu_burst++;
	    myproc()->total_time++;
	    update_cpu_wait(); 
	    update_aging();
	}

	if (myproc()->state == SLEEPING) {
	    myproc()->io_wait_time++;
        }
    }
  
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // terminate process
  if(myproc() && myproc()->state == RUNNING && myproc()->end_time != -1 && myproc()->total_time >= myproc()->end_time) {
   #ifdef DEBUG
          cprintf("PID: %d uses %d ticks in mlfq[%d], total(%d/%d)\n", myproc()->pid, myproc()->cpu_burst, myproc()->q_level, myproc()->total_time, myproc()->end_time);
          cprintf("PID: %d, used %d ticks. terminated\n", myproc()->pid, myproc()->end_time);
   #endif
    exit();
  }

  if(myproc() && myproc()->state == RUNNING && myproc()->end_time != -1 && myproc()->cpu_burst >= get_time_quantum(myproc()->q_level)) {
    #ifdef DEBUG
       cprintf("PID: %d uses %d ticks in mlfq[%d], total(%d/%d)\n", myproc()->pid, get_time_quantum(myproc()->q_level), myproc()->q_level, myproc()->total_time, myproc()->end_time);
    #endif

    if(myproc()->q_level < 3) {
      // Initialize cpu_wait and io_wait_time only if the priority is down
      myproc()->q_level++;
      myproc()->cpu_wait = 0;
    } 

    yield();
  }


  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  //if(myproc() && myproc()->state == RUNNING &&
    // tf->trapno == T_IRQ0+IRQ_TIMER)
    //yield();

  // Check if the process has been killed since we yielded
 if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
   exit();
}
