/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/
/* 

  my very own cute malloc prog

  slow as hell, but nice if you are on ULTRIX workstation, cos
  the built in malloc sucks.

*/

#include <malloc.h>

#include "config.h"
#include "player.h"

#define HOLDING_BLOCK_SIZE 100000

#define MAX_SMALL_BLOCK_SIZE 95000

struct m_info {
  struct m_info *next;
  int total_free;
  int total_used;
  int biggest_space;
  int first;
};

typedef struct m_info minfo;

extern void handle_error();
extern char *end_string();

int total_mallocs=0,small_mallocs=0,total_malloced=0,average_size=0;
int max_block=0,smallest_block=MAX_SMALL_BLOCK_SIZE,total_free=0;
minfo *mstart;

/* initialise the malloc routines */

void init_malloc()
{
  mstart=0;
}


/* show some stats */

void malloc_stats(player *p,char *str)
{
  char *oldstack;
  minfo *scan;
  int holding_blocks=0,in_use=0,free_space=0;
  oldstack=stack;
  
  for(scan=mstart;scan;scan=scan->next) {
    holding_blocks++;
    in_use += scan->total_used;
    free_space += scan->total_free;
  }

  sprintf(stack,"--- Malloc Stats\n"
	  "Total Requests         %d\n"
	  "Total Small Requests   %d\n"
	  "Total Frees            %d\n"
	  "Total Malloced Space   %d\n"
	  "Average small malloc   %d\n"
	  "Max small malloc       %d\n"
	  "Min small malloc       %d\n"
	  "Holding blocks         %d\n"
	  "Total small use        %d\n"
	  "Total small free       %d\n",
	  total_mallocs,small_mallocs,total_free,total_malloced,
	  average_size,max_block,smallest_block,holding_blocks,
	  in_use,free_space);
  stack=end_string(stack);
  tell_player(p,oldstack);
  stack=oldstack;
}

/* display stats for each block */

void block_stats(player *p,char *str)
{
  char *oldstack;
  minfo *scan;
  int number=1;

  oldstack=stack;

  for(scan=mstart;scan;scan=scan->next) {
    sprintf(stack,"[%d] U - %d, F - %d, B - %d\n",
	    number,scan->total_used,scan->total_free,scan->biggest_space);
    while(*stack) stack++;
    number++;
  }
  *stack++=0;
  tell_player(p,oldstack);
  stack=oldstack;
}



/* find out if a given address is a small block or not */

minfo *find_block(void *where)
{
  minfo *scan;
  int test,top,bottom;
  scan=mstart;
  test=(int)where;
  while(scan) {
    bottom=(int) (&(scan->first));
    top=bottom+HOLDING_BLOCK_SIZE-sizeof(minfo);
    if ((test>=bottom) && (test<=top)) return scan;
    scan=scan->next;
  }
  return 0;
}


/* find a block that has enough space for malloc
   if not then create a new one */

minfo *get_block(int size)
{
  minfo *scan,*best_block=0;
  int smallest_free=HOLDING_BLOCK_SIZE;
  for(scan=mstart;scan;scan=scan->next) 
    if (((scan->biggest_space)>size) && ((scan->biggest_space)<smallest_free)) {
      smallest_free=scan->biggest_space;
      best_block=scan;
    }
  if (best_block) return best_block;
  printf("requesting an extra %d ...\n",HOLDING_BLOCK_SIZE);
  scan=(minfo *)malloc(HOLDING_BLOCK_SIZE);
  memset((void *)scan,0,HOLDING_BLOCK_SIZE);
  scan->next=mstart;
  mstart=scan;
  scan->total_free=HOLDING_BLOCK_SIZE-sizeof(minfo)-sizeof(int);
  scan->total_used=0;
  scan->biggest_space=scan->total_free;
  scan->first=-(scan->total_free);
  return scan;
}


/* find the best suited space in the block */

void *get_space(minfo *block,int needed)
{
  void *scan,*best_space=0;
  int size,previous=HOLDING_BLOCK_SIZE;
  scan=&(block->first);
  do {
    size=*((int *)scan);
    if (size<0) {
      size = -size;
      if ((size>=needed) && (size<previous)) {
	best_space=scan;
	previous=size;
      }
    } 
    scan+=size+sizeof(int);
  } while(size);
  if (!best_space) handle_error("bad malloc block");
  return best_space;
}

/* find out biggest free space in block */

int recheck_biggest(minfo *block)
{
  void *scan;
  int size,biggest_free=0;
  scan=&(block->first);
  do {
    size=*((int *)scan);
    if (size<0) {
      size = -size;
      if (size>biggest_free) biggest_free=size;
    }
    scan+=size+sizeof(int);
  } while(size);
  return biggest_free;
}



void *my_malloc(int size)
{
  minfo *block;
  void *where;
  char *fill;
  int i,old,new;
  total_mallocs++;
  if (size>MAX_SMALL_BLOCK_SIZE) return (void *)malloc(size);
  size = (size+15)&(-16);
  if (size<16) size=16;

  small_mallocs++;
  total_malloced += size;
  average_size = (average_size+size)>>1;
  if (size>max_block) max_block=size;
  if (size<smallest_block) smallest_block=size;

  block=get_block(size);
  where=get_space(block,size);
  old=*((int *)where);
  fill=(char *)(where+sizeof(int)+size);
  new=old+size+sizeof(int);
  if (new<0) {
    *((int *)fill) = new;
    block->total_free -= sizeof(int);
    block->total_used += sizeof(int);
  }
  else size=-old;
  *((int *)where) = size;
  where += sizeof(int);  

  block->total_free -= size;
  block->total_used += size;
  block->biggest_space = recheck_biggest(block);

/*  printf("%d %d [%d]\n",block->total_free,block->total_used,block->biggest_space); */
  return where;
}



/* my free function */

void my_free(void *where)
{
  minfo *block;
  void *next,*previous=0;
  int size=1,s,new;

  total_free++;

  block=find_block(where);
  if (!block) {
    free(where);
    return;
  }
  where-=sizeof(int);
  next=&(block->first);
  while(size && (next!=where)) {
    previous=next;
    size=*((int *)next);
    if (size<0) size = -size;
    next+=size+sizeof(int);
  }
  if (!size) handle_error("bad malloc area");
  size=*((int *)where);
  if (size<0) {
    log("malloc","Tried to free already free space");
    return;
  }
  new=size;
  next=where+size+sizeof(int);
  s=*((int *)next);
  if (s<0) {
    new-=(s-sizeof(int));
    size+=sizeof(int);
  }
  if (previous) {
    s=*((int *)previous);
    if (s<0) {
      new-=(s-sizeof(int));
      size+=sizeof(int);
      *((int *)previous)=-new;
    }
    else *((int *)where)=-new;
  }
  else *((int *)where)=-new;
  
  block->total_free+=size;
  block->total_used-=size;
  
  if (size>(block->biggest_space)) block->biggest_space=size;
}














/* malloc data */

void show_malloc(player *p,char *str)
{
  char *oldstack;
  struct mallinfo i;
  
  oldstack=stack;
  i=mallinfo();
  
  sprintf(stack,"Total arena space\t%d\n"
	  "Ordinary blocks\t\t%d\n"
	  "Small blocks\t\t%d\n"
	  "Holding blocks\t\t%d\n"
	  "Space in headers\t\t%d\n"
	  "Small block use\t\t%d\n"
	  "Small blocks free\t%d\n"
	  "Ordinary block use\t%d\n"
	  "Ordinary block free\t%d\n"
#ifdef ULTRIX
	  "Keep cost\t\t%d\n",
#else
	  "Keep cost\t\t%d\n"
	  "Small block size\t\t%d\n"
	  "Small blocks in holding\t%d\n"
	  "Rounding factor\t\t%d\n"
	  "Ordinary block space\t%d\n"
	  "Ordinary blocks alloc\t%d\n"
	  "Tree overhead\t%d\n",
#endif
          i.arena,i.ordblks,i.smblks,i.hblks,i.hblkhd,i.usmblks,
#ifdef ULTRIX
          i.fsmblks,i.uordblks,i.fordblks,i.keepcost);
#else
          i.fsmblks,i.uordblks,i.fordblks,i.keepcost,
          i.mxfast,i.nlblks,i.grain,i.uordbytes,i.allocated,i.treeoverhead);
#endif
  stack=end_string(stack);
  tell_player(p,oldstack);
  stack=oldstack;
}
