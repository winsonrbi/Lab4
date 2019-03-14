#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
  uint size = myproc()->sz;
  int i;
  if(id <= 0){//invalid ID
    return -1;
  }
  acquire(&(shm_table.lock));
    for(i=0;i<64;i++){ //look for ID
      if(id == shm_table.shm_pages[i].id){
        if(shm_table.shm_pages[i].refcnt <= 0 || shm_table.shm_pages[i].frame == 0){
          release(&(shm_table.lock));
          return -1;
        }
        //ELSE MAP page
        if(mappages(myproc()->pgdir,(char*)PGROUNDUP(size),PGSIZE,V2P(shm_table.shm_pages[i].frame),(PTE_W|PTE_U))==-1){
          release(&(shm_table.lock));
          return -1;
        }
        //ELSE done successsfully
        shm_table.shm_pages[i].refcnt++;
        *pointer=(char*)size;
        release(&(shm_table.lock));
        return 0;
      }
    }
    //ID is not FOUND
    for(i=0;i<64;i++){
    if(shm_table.shm_pages[i].id == 0){ //Unused page found
      shm_table.shm_pages[i].frame = kalloc();
      memset(shm_table.shm_pages[i].frame,0,PGSIZE);
      if(mappages(myproc()->pgdir,(char*)PGROUNDUP(size),PGSIZE,V2P(shm_table.shm_pages[i].frame),(PTE_W|PTE_U))==-1){
        release(&(shm_table.lock));
        return -1;
      }
      //ELSE done successsfully
      shm_table.shm_pages[i].id = id;
      if(shm_table.shm_pages[i].refcnt != 0){
        release(&(shm_table.lock));
        return -1;
      }
      shm_table.shm_pages[i].refcnt++;
      *pointer=(char*)size;
      release(&(shm_table.lock));
      return 0;
    }
  }
  release(&(shm_table.lock));//NO free pages
  return -1; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
  if(id <= 0){
    return -1;
  }
  int i;
  acquire(&(shm_table.lock));
  for(i=0;i<64;i++){ //Find page with ID
    if(id == shm_table.shm_pages[i].id){
      if(shm_table.shm_pages[i].refcnt == 0){//Error Just In Case
        release(&(shm_table.lock));
        return -1;
      }
      shm_table.shm_pages[i].refcnt--;
      if(shm_table.shm_pages[i].refcnt == 0){ //if page is not being used anymore
        shm_table.shm_pages[i].id =0;
        shm_table.shm_pages[i].frame =0;
      }
      release(&(shm_table.lock));
      return 0;
    }
  }
  //ID NOT FOUND
  release(&(shm_table.lock));
  return -1; //added to remove compiler warning -- you should decide what to return
}
