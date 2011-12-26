// http://groups.google.com/group/ml-devel/browse_thread/thread/26cb46acd262b953
// Author: Arm.Indy
// 550D only

#include"dryos.h"
#include"debug.h"

/*
 * dryos shell debug functions
 */


struct make_cfg {
   const char* dry_ver;            // offset 0, "%s"
   const char* dry_mk;             // offset 4
   unsigned int    act_spi_sem;        // offset 8, "%8d"
   unsigned int    act_spi_event;      // offset 0x0c
   unsigned int    act_spi_mq;          // offset 0x10
   unsigned int    act_spi_mutex;      // offset 0x14
   unsigned int    act_spi_cond;       // offset 0x18
   unsigned int    act_spi_timer;      // offset 0x1c
   unsigned int    act_spi_clock;       // offset 0x20
   unsigned int    act_spi_mem;        // offset 0x24
   unsigned int    act_spi_isr;        // offset 0x28
   unsigned int    act_spi_objlist;    // offset 0x2c
   unsigned int    act_spi_objinfo;     // offset 0x30
   unsigned int    act_spi_objsetname;
   unsigned int    act_timeout;
   unsigned int    act_objname;
   unsigned int    dbg_stack_check;    // offset 0x40
   unsigned int    dbg_error_check;
   unsigned int    dbg_logging;
   unsigned int    sys_mem_start;      // offset 0x4c, "0x%08lx"
   unsigned int    sys_mem_max;        // offset 0x50, "%8ld"
   unsigned int    user_mem_start;     // 0x%08lx
   unsigned int    user_mem_max;       // %8ld
   unsigned int    sys_objs_start;     // 0x%08lx
   unsigned int    sys_objs_end;       // offset 0x60, 0x%08lx
   unsigned int    priority_max;       // offset 0x64
   unsigned int    task_max;
   unsigned int    semaphore_max;
   unsigned int    event_max;
   unsigned int    message_q_max;      // offset 0x74
   unsigned int    mutex_max;
   unsigned int    condition_max;
   unsigned int    timer_max;
   unsigned int    vector_max;         // offset 0x84
   unsigned int    it4_mbx_max;
   unsigned int    it4_mpf_max;
   unsigned int    it4_mpl_max;
   unsigned int    level_low;          // offset 0x94
   unsigned int    level_timer;
   unsigned int    level_kern;
   unsigned int    prio_default;
   unsigned int    stack_default;      // offset 0xa4
   unsigned int    stack_idle;
   unsigned int    stack_init;
   unsigned int    stack_addr_idle;
   unsigned int    stack_addr_init;    // offset 0xb4, 0x%08lx
};


void debug_mkcfg() { // sub_FF01389C in 550d_109

  int (*get_mkcfg)(struct make_cfg* buffer) =  (void*)0xFF01757C;

  struct make_cfg mkcfg_s;

  get_mkcfg(&mkcfg_s);

  DebugMsg( DM_MAGIC, 3, "vers_dry                 %s", mkcfg_s.dry_ver);
  DebugMsg( DM_MAGIC, 3, "vers_mk                  %s", mkcfg_s.dry_mk);

  DebugMsg( DM_MAGIC, 3, "act_spi_sem       %8d", mkcfg_s.act_spi_sem);
  DebugMsg( DM_MAGIC, 3, "act_spi_event     %8d", mkcfg_s.act_spi_event);
  DebugMsg( DM_MAGIC, 3, "act_spi_mq        %8d", mkcfg_s.act_spi_mq);
  DebugMsg( DM_MAGIC, 3, "act_spi_mutex     %8d", mkcfg_s.act_spi_mutex);
  DebugMsg( DM_MAGIC, 3, "act_spi_cond      %8d", mkcfg_s.act_spi_cond);
  DebugMsg( DM_MAGIC, 3, "act_spi_timer     %8d", mkcfg_s.act_spi_timer);
  DebugMsg( DM_MAGIC, 3, "act_spi_clock     %8d", mkcfg_s.act_spi_clock);
  DebugMsg( DM_MAGIC, 3, "act_spi_mem       %8d", mkcfg_s.act_spi_mem);
  DebugMsg( DM_MAGIC, 3, "act_spi_isr       %8d", mkcfg_s.act_spi_isr);
  DebugMsg( DM_MAGIC, 3, "act_spi_objlist   %8d", mkcfg_s.act_spi_objlist);
  DebugMsg( DM_MAGIC, 3, "act_spi_objinfo   %8d", mkcfg_s.act_spi_objinfo);
  DebugMsg( DM_MAGIC, 3, "act_spi_objsetname%8d", mkcfg_s.act_spi_objsetname);
  DebugMsg( DM_MAGIC, 3, "act_timeout       %8d", mkcfg_s.act_timeout);
  DebugMsg( DM_MAGIC, 3, "act_objname       %8d", mkcfg_s.act_objname);

  DebugMsg( DM_MAGIC, 3, "dbg_stack_check   %8d", mkcfg_s.dbg_stack_check);
  DebugMsg( DM_MAGIC, 3, "dbg_error_check   %8d", mkcfg_s.dbg_error_check);
  DebugMsg( DM_MAGIC, 3, "dbg_logging       %8d", mkcfg_s.dbg_logging);
  DebugMsg( DM_MAGIC, 3, "sys_mem_start   0x%08lx", mkcfg_s.sys_mem_start);
  DebugMsg( DM_MAGIC, 3, "sys_mem_max       %8ld", mkcfg_s.sys_mem_max);
  DebugMsg( DM_MAGIC, 3, "user_mem_start  0x%08lx", mkcfg_s.user_mem_start);
  DebugMsg( DM_MAGIC, 3, "user_mem_max      %8ld", mkcfg_s.user_mem_max);
  DebugMsg( DM_MAGIC, 3, "sys_objs_start  0x%08lx", mkcfg_s.sys_objs_start);
  DebugMsg( DM_MAGIC, 3, "sys_objs_end    0x%08lx", mkcfg_s.sys_objs_end);

  DebugMsg( DM_MAGIC, 3, "priority_max      %8d", mkcfg_s.priority_max);
  DebugMsg( DM_MAGIC, 3, "task_max          %8d", mkcfg_s.task_max);
  DebugMsg( DM_MAGIC, 3, "semaphore_max     %8d", mkcfg_s.semaphore_max);
  DebugMsg( DM_MAGIC, 3, "event_max         %8d", mkcfg_s.event_max);
  DebugMsg( DM_MAGIC, 3, "message_q_max     %8d", mkcfg_s.message_q_max);
  DebugMsg( DM_MAGIC, 3, "mutex_max         %8d", mkcfg_s.mutex_max);
  DebugMsg( DM_MAGIC, 3, "condition_max     %8d", mkcfg_s.condition_max);
  DebugMsg( DM_MAGIC, 3, "timer_max         %8d", mkcfg_s.timer_max);
  DebugMsg( DM_MAGIC, 3, "vector_max        %8d", mkcfg_s.vector_max);

  DebugMsg( DM_MAGIC, 3, "it4_mbx_max       %8d", mkcfg_s.it4_mbx_max);
  DebugMsg( DM_MAGIC, 3, "it4_mpf_max       %8d", mkcfg_s.it4_mpf_max);
  DebugMsg( DM_MAGIC, 3, "it4_mpl_max       %8d", mkcfg_s.it4_mpl_max);

  DebugMsg( DM_MAGIC, 3, "level_low         %8d", mkcfg_s.level_low);
  DebugMsg( DM_MAGIC, 3, "level_timer       %8d", mkcfg_s.level_timer);
  DebugMsg( DM_MAGIC, 3, "level_kern        %8d", mkcfg_s.level_kern);

  DebugMsg( DM_MAGIC, 3, "prio_default      %8d", mkcfg_s.prio_default);
 
  DebugMsg( DM_MAGIC, 3, "stack_default     %8ld", mkcfg_s.stack_default);
  DebugMsg( DM_MAGIC, 3, "stack_idle        %8ld", mkcfg_s.stack_idle);
  DebugMsg( DM_MAGIC, 3, "stack_init        %8ld", mkcfg_s.stack_init);
  DebugMsg( DM_MAGIC, 3, "stack_addr_idle 0x%08lx", mkcfg_s.stack_addr_idle);
  DebugMsg( DM_MAGIC, 3, "stack_addr_init 0x%08lx", mkcfg_s.stack_addr_init);

}

void debug_ps() 
{ // do not work. should list tasks and their attributes

  unsigned int a=0;
  unsigned int b;
  int n, r;
  unsigned int c;
  unsigned int t[0x60];
  int i;
  unsigned int *addr;

  // see extask 0xff015050

  int (*get_max_task)(unsigned int*, unsigned int*) =  (void*)0xFF0154b0;
  int (*get_task_info)(int *, unsigned int *) =  (void*)0xFF017b4c;

  n = get_max_task(&a, &b);

  DebugMsg( DM_MAGIC, 3, "get_max_task: arg1=0x%08x, arg2=0x%08x, n=0x%08x", a, b, n);
  c = 0;
  for (i=0; i<0x60; i++)
    t[i]=0xdeadbeef;
  for(addr=a; addr<b; addr++)
    DebugMsg( DM_MAGIC, 3, "0x%x = 0x%08x", addr, *addr);

  r = get_task_info( &c, t); // r == -1 means error
  DebugMsg( DM_MAGIC, 3, "r=0x%x, c=0x%x", r, c);
  r = get_task_info( &a, &c); // r == -1 means error
  DebugMsg( DM_MAGIC, 3, "r=0x%x, a=0x%x", r, c);

  for (i=0; i<0x60; i++)
    DebugMsg( DM_MAGIC, 3, "t[%2d]=0x%08x", i, t[i]);

  /*
  for (i=0; i<n; i++) {
    r = get_task_info( i, t); // r == -1 means error
    DebugMsg( DM_MAGIC, 3, "get_task_info: arg1=0x%08x, r=0x%x", i, r);
    if (r==0 && !c) {
      for (i=0; i<0x60; i++)
        DebugMsg( DM_MAGIC, 3, "t[%2d]=0x%08x", i, t[i]);
      c = 1;
    }
  
  }*/
 
  /*
  for (addr=a; addr<b; addr++)
    DebugMsg( DM_MAGIC, 3, "0x%08x = 0x%08x", addr, *addr);
  */
  char ps_str[] = " NAME            ID   STATE PRI         WAIT(ID)      STACK  %        SP";
}


struct mem_info_str {
  unsigned int    start_addr;
  unsigned int    end_addr;
  unsigned int    total_size;   
  unsigned int    alloc_size;
  unsigned int    alloc_peak;
  unsigned int    alloc_count;
  unsigned int    free_size;
  unsigned int    freeblock_maxsize;
  unsigned int    freeblock_count;
  unsigned int    extend_size;
}; // size = 40

void debug_sysmem() // ff014244. "meminfo -s" = system memory information
{

  int (*get_mem_info)(struct mem_info_str*) =  (void*)0xFF017700;

  struct mem_info_str meminfo;
  int i;

  get_mem_info(&meminfo);

  DebugMsg( DM_MAGIC, 3,"System Memory Information");
  DebugMsg( DM_MAGIC, 3,"  Start Address       = 0x%08lx", meminfo.start_addr);
  DebugMsg( DM_MAGIC, 3,"  End Address         = 0x%08lx", meminfo.end_addr);
  DebugMsg( DM_MAGIC, 3,"  Total Size          = 0x%08x (%9d)", meminfo.total_size, meminfo.total_size);
  DebugMsg( DM_MAGIC, 3,"  Allocated Size      = 0x%08x (%9d)", meminfo.alloc_size, meminfo.alloc_size);
  DebugMsg( DM_MAGIC, 3,"  Allocated Peak      = 0x%08x (%9d)", meminfo.alloc_peak, meminfo.alloc_peak);
  DebugMsg( DM_MAGIC, 3,"  Allocated Count     = 0x%08x (%9d)", meminfo.alloc_count, meminfo.alloc_count);
  DebugMsg( DM_MAGIC, 3,"  Free Size           = 0x%08x (%9d)", meminfo.free_size, meminfo.free_size);
  DebugMsg( DM_MAGIC, 3,"  Free Block Max Size = 0x%08x (%9d)", meminfo.freeblock_maxsize, meminfo.freeblock_maxsize);
  DebugMsg( DM_MAGIC, 3,"  Free Block Count    = 0x%08x (%9d)", meminfo.freeblock_count, meminfo.freeblock_count);
  DebugMsg( DM_MAGIC, 3,"  Extendable Size     = 0x%08x (%9d)", meminfo.extend_size, meminfo.extend_size);

}

struct malloc_mem_str {
  unsigned int start_addr;
  unsigned int end_addr;
  unsigned int total_size;
  unsigned int alloc_size;
  unsigned int alloc_peak;
  unsigned int alloc_count;
  unsigned int    free_size;
  unsigned int    freeblock_maxsize;
  unsigned int    freeblock_count;
  unsigned int    type;
}; // size = 40

void debug_malloc_mem() // FF013F5C. "meminfo -m" = malloc memory information
{

  int (*get_malloc_info)(unsigned int*) =  (void*)0xFF07B614;

  unsigned int t[10];
  char *typestr;
  struct malloc_mem_str meminfo;

  get_malloc_info(&meminfo);

  switch(meminfo.type) {
  case 3: typestr="sbrk"; break;
  case 2: typestr="onetime"; break;
  default: typestr="null";
  }
    
  DebugMsg( DM_MAGIC, 3,"Malloc Information (%s type)", typestr);
  DebugMsg( DM_MAGIC, 3,"  Start Address       = 0x%08lx", meminfo.start_addr);
  DebugMsg( DM_MAGIC, 3,"  End Address         = 0x%08lx", meminfo.end_addr);
  DebugMsg( DM_MAGIC, 3,"  Total Size          = 0x%08x (%9d)", meminfo.total_size, meminfo.total_size);
  DebugMsg( DM_MAGIC, 3,"  Allocated Size      = 0x%08x (%9d)", meminfo.alloc_size, meminfo.alloc_size);
  DebugMsg( DM_MAGIC, 3,"  Allocated Peak      = 0x%08x (%9d)", meminfo.alloc_peak, meminfo.alloc_peak);
  DebugMsg( DM_MAGIC, 3,"  Allocated Count     = 0x%08x (%9d)", meminfo.alloc_count, meminfo.alloc_count);
  DebugMsg( DM_MAGIC, 3,"  Free Size           = 0x%08x (%9d)", meminfo.free_size, meminfo.free_size);
  DebugMsg( DM_MAGIC, 3,"  Free Block Max Size = 0x%08x (%9d)", meminfo.freeblock_maxsize, meminfo.freeblock_maxsize);
  DebugMsg( DM_MAGIC, 3,"  Free Block Count    = 0x%08x (%9d)", meminfo.freeblock_count, meminfo.freeblock_count);
  DebugMsg( DM_MAGIC, 3,"  Type                = %8d", meminfo.type);


}


#define BSS_START 0x27128
#define HEAP_START 0x8A9E0
#define HEAP_END 0x103958

void debug_memmap() // see FF014B2C memmap
{
  DebugMsg( DM_MAGIC, 3,"== ITCM ==");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x0000, "DRY_VECTOR_ADDR");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x04b0, "DRY_HANDLER_ADDR");
  unsigned int l = 0xFF0103F8 - 0xFF0101E4;
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", l, l);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", l+0x4b0, "DRY_HANDLER_END_ADDR");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", l+0x4b0, "DRY_IRQ_STACK_START");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", l+0x4b0, "DRY_RESET_STACK_START");
  l = 0x1000-l-0x4b0;
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", l, l); // error

  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x1000, "DRY_IRQ_STACK");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x1000, "DRY_RESET_STACK");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x1000, "ITCM_END_ADDR");

  DebugMsg( DM_MAGIC, 3,"== DTCM ==");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x400006F8, "DRY_VECTOR_FUNC");
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", 0x400, 0x400);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x40000AF8, "DRY_VECTOR_ARG");
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", 0x400, 0x400);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x40001000, "DTCM_END_ADDR");

  DebugMsg( DM_MAGIC, 3,"== ROM  ==");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0xFF810000, "text start");
  unsigned int v = 0xFF547F60 + 0x7F0000;
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%ul)", v, v); 
  DebugMsg( DM_MAGIC, 3,"%08x : %s", v, "romdata start");
  unsigned int v2 = BSS_START-0x1900;
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", v2, v2);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", v+v2, "romdata end");

  DebugMsg( DM_MAGIC, 3,"== DRAM ==");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x1900, "data start");
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", v2, v2);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", BSS_START, "bss start");
  v2 = HEAP_START-BSS_START;
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", v2, v2);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", HEAP_START, "heap start");
  v2 = HEAP_END-HEAP_START;
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", v2, v2);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", HEAP_END, "DRY_HEAP_END");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", HEAP_END, "DRY_SYS_OBJS_START");
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", 0x72A8, 0x72A8);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x10AC00, "DRY_SYS_MEM_START");
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", 0x104000, 0x104000);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x20EC00, "DRY_ERREX_STACK_START");
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", 0x400, 0x400);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x20F000, "DRY_ERREX_STACK");
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x20F000, "DRY_EXCEP_STACK_START");
  DebugMsg( DM_MAGIC, 3,"           0x%08x(%d)", 0x1000, 0x1000);
  DebugMsg( DM_MAGIC, 3,"%08x : %s", 0x210000, "DRY_EXCEP_STACK");

}


debug_objinfo() // see FF014528
{
  unsigned int t[11*3];
  char *objname[]= {"task  ", "sem   ", "event ", "mq    ", "mb    ", "mutex ", "cond  ", "timer ", "?     ", "?     ", "?     " };
  int i, n;
  unsigned long *addr;

  int (*get_objinfo)(unsigned int) =  (void*)0xFF079948;

  n = get_objinfo(t);

  DebugMsg( DM_MAGIC, 3, "%s %6s %6s %6s", "      ", "MAX   ", "COUNT ", "PEAK  ");
  for (i=0; i<11; i++)
    DebugMsg( DM_MAGIC, 3, "%s %6d %6d %6d", objname[i], t[i*3],t[(i*3)+1],t[(i*3)+2]);

}

debug_objsize() // FF2EBB48 mkobjsize_cmd 
{
  DebugMsg( DM_MAGIC, 3, " sizeof(%9s) = %3d", "dlqueue_t", 8);
  DebugMsg( DM_MAGIC, 3, " sizeof(%9s) = %3d", "task_t", 0x54);
  DebugMsg( DM_MAGIC, 3, " sizeof(%9s) = %3d", "sem_t", 0x18);
  DebugMsg( DM_MAGIC, 3, " sizeof(%9s) = %3d", "event_t", 0x18);
  DebugMsg( DM_MAGIC, 3, " sizeof(%9s) = %3d", "mq_t", 0x2C);
  DebugMsg( DM_MAGIC, 3, " sizeof(%9s) = %3d", "mutex_t", 0x1C);
  DebugMsg( DM_MAGIC, 3, " sizeof(%9s) = %3d", "timer_t", 0x28);

}

// FF1EBD04 obj_info_cmd 
/*
ROM:FF2EA134 task_cmd
FF2E9C14 display_tasks_maybe

 */

/*
ROM:FF2EA5F0 sem_cmd                                 ; DATA XREF: obj_info_cmd+30o
ROM:FF2EA5F0                                         ; ROM:off_FF1EBE54o
ROM:FF2EA5F0                 MOV     R2, #0
ROM:FF2EA5F4                 B       sem_list

*/

struct task_attr_str {
  unsigned int entry;
  unsigned int args;
  unsigned int stack;
  unsigned int size;
  unsigned int used; // 0x10
  unsigned int name;
  unsigned int off_18;
  unsigned int flags;
  unsigned char wait_id;
  unsigned char pri;
  unsigned char state;
  unsigned char fpu;
  unsigned int id;
}; // size = 0x28


void debug_task()
{
  int i, c;
  unsigned int r;
  struct task_attr_str task_attr;
  char *name, *args;
  unsigned int *task_max=0x3080;

  //DebugMsg( DM_MAGIC, 3, "      ID   STATE PRI         WAIT(ID)  FLAGS     ENTRY(ARGUMENT)     STACK   SIZE   USED  NAME");

  int (*is_taskid_valid)(int, int, unsigned int) =  (void*)0xFF2e9ab0;
  int (*get_obj_attr)(void*, unsigned char*, int, int) =  (void*)0xFF079ce4;

  // wait_id: 0=sleep, 1=sem, 2=flg/event, 3=sendmq, 4=recvmq, 5=mutex
  // state: 0=ready, 1=wait, 2=susp, other=wait+s

  c = 1;
  DebugMsg( DM_MAGIC, 3, "*task_max=%d", *task_max);
  for (c=1; c<(*task_max); c++) {
    r = is_taskid_valid(1, c, &task_attr); // ok
    if (r==0) {
      r = get_obj_attr( &(task_attr.args), &(task_attr.fpu), 0, 0); // buggy ?
      if (task_attr.name!=0)
  name=task_attr.name;
      else
  name="?";
     DebugMsg( DM_MAGIC, 3, "c=%02d. entry=0x%08x, sp=0x%08x, size=%4d, used=%4d", 
    c, task_attr.entry, task_attr.stack, task_attr.size, task_attr.used);
      DebugMsg( DM_MAGIC, 3, "name= %s, args= 0x%08x", name, task_attr.args); 
      DebugMsg( DM_MAGIC, 3, "  _18=0x%08x, flags=0x%08x, wid=%d, pri=%d, state=%d", 
    task_attr.off_18, task_attr.flags, task_attr.wait_id, task_attr.pri, task_attr.state);
      DebugMsg( DM_MAGIC, 3, "  fpu=%d, ID=0x%08x", task_attr.fpu, task_attr.id);
    }
  }

}

void debug_dminfo() // FF1DBE88 dminfo_cmd
{
  unsigned int t[0x100];
  unsigned int t2[7];

  int (*get_maxdrivers)(unsigned int*) =  (void*)0xFF07348c;
  int (*drivers_suspend_maybe)() =  (void*)0xFF073770;
  int (*drivers_resume_maybe)() =  (void*)0xFF07379c;
  int (*get_drivers_info)(unsigned int*) =  (void*)0xFF0737bc;
  int i;
  unsigned int r;

  for (i=0; i<0x100; i++)
    t[i]=0xdeadbeef;

  DebugMsg( DM_MAGIC, 3, "[Driver Entry]");
  DebugMsg( DM_MAGIC, 3, "  total : %3d (DRV_ENTRY_MAX)", 5);

  r = get_maxdrivers( t2); 
  DebugMsg( DM_MAGIC, 3, "  used  : %3d", t2[0]);

  DebugMsg( DM_MAGIC, 3, "[Created Devices]");
  DebugMsg( DM_MAGIC, 3, "  drvNo  name");

  return; // next code do not work


  r = drivers_suspend_maybe();
  DebugMsg( DM_MAGIC, 3, "  drivers_suspend_maybe=%d", r);

  r = get_drivers_info(t);
  DebugMsg( DM_MAGIC, 3, "  get_drivers_info=%d", r);
  while (r==0) {
    for (i=0; i<0x100; i++)
      DebugMsg( DM_MAGIC, 3, "t[%2d]=0x%08x", i, t[i]);
    DebugMsg( DM_MAGIC, 3, "   %3d   %s", t[0], t[1]);
    r = get_drivers_info(t);
    DebugMsg( DM_MAGIC, 3, "  get_drivers_info=%d", r);
  }

  //  r = drivers_resume_maybe();
  DebugMsg( DM_MAGIC, 3, "  drivers_resume_maybe=%d", r);

}
