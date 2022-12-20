#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#define debug

#define MAX_SCAN_PAGES (sizeof(int) * 8)
#define FILL sizeof(unsigned int)
uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  //args from process
  uint64 startVirtualAddr;
  uint64 currentAddr = 0;
  uint64 mask;
  unsigned int returnMask = 0;
  int numberOfPages;
  int i = 0;
  struct proc* p = myproc();
  pagetable_t pagetable = p->pagetable;
  pte_t *pte;
  
  //get params
  argaddr(0, &startVirtualAddr);
  argint(1, &numberOfPages);
  argaddr(2, &mask);

  //define upper bound if scan pages
  //note that returnMask is uint64, so we can only moniter 64 pages at most.
  if(numberOfPages > MAX_SCAN_PAGES)
    numberOfPages = MAX_SCAN_PAGES;
  
  currentAddr = startVirtualAddr;

  while(i < numberOfPages){
  //error message from vm.c walk
    if(currentAddr >= MAXVA)
      panic("pgaccess");
    pagetable = p->pagetable;
    //seems that level begins from 2 in walk()
    for(int level = 2; level > 0; level--) {
      pte = &pagetable[PX(level, currentAddr)];
      //dfs to get next level
      if(*pte & PTE_V) {
        pagetable = (pagetable_t)PTE2PA(*pte);
      } 
      else {
        //if not allocated, return -1
        return -1;
      }
    }
    //get page index from pagetable. level == 0.
    pte = &pagetable[PX(0, currentAddr)];
    //judge if PTE_V and PTE_U is valid
    if(pte == 0){
      return -1;
    
    }
    else if((*pte & PTE_V) == 0){
      return -1;
    }
    else if((*pte & PTE_U) == 0){
      return -1;
    }
    //if PTE_A is true
    else if(*pte & PTE_A){
      //set ith page to be true and set PTE_A 0
      returnMask = returnMask | (1L << i);
      *pte = *pte & (~PTE_A);
    }
    currentAddr += PGSIZE;
    i++;
  }
  //output result with copyout
  copyout(p->pagetable, mask, (char*)(&returnMask), FILL);
  return 0;
}
#endif