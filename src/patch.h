/* Memory patching */

/* Features:
 * - Keep track of all memory patches
 * - Patch either a single address or an array/matrix of related memory addresses
 * - Undo the patches (built-in undo storage for simple patches, user-supplied storage for array/matrix patches)
 * - Menu display
 */

/* Design goals:
 * - Traceability: be able to see and review all patched addresses from menu
 * - Safety checking: do not patch if the memory contents is not what you expected
 * - Minimally invasive: only lock down cache when there's some ROM address to patch, and unlock it when it's no longer needed
 * - Troubleshooting: automatically check whether the patch is still active or it was overwritten
 * - Versatility: cover a wide range of situations (bit patching, scaling, offset) with the same API
 * 
 * Please do not patch memory directly; use these functions instead (especially for patches that can be undone at runtime).
 * RAM patches applied at boot time can be hardcoded for now (this may change).
 * ROM patches must be always applied via this library.
 * 
 * Long-term goal: any patch that changes Canon functionality should be applied via this library.
 * (including boot patches, state object hooks, any other hooks done by direct memory patching).
 */

#ifndef _patch_h_
#define _patch_h_

#define E_PATCH_OK 0
#define E_PATCH_UNKNOWN_ERROR       0x1
#define E_PATCH_ALREADY_PATCHED     0x2
#define E_PATCH_TOO_MANY_PATCHES    0x4
#define E_PATCH_OLD_VALUE_MISMATCH  0x8
#define E_PATCH_CACHE_COLLISION     0x10
#define E_PATCH_CACHE_ERROR         0x20
#define E_PATCH_REG_NOT_FOUND       0x40

#define E_UNPATCH_OK                0
#define E_UNPATCH_NOT_PATCHED       0x10000
#define E_UNPATCH_OVERWRITTEN       0x20000
#define E_UNPATCH_REG_NOT_FOUND     0x80000

/****************
 * Data patches *
 ****************/

/* simple data patch */
int patch_memory(
    uintptr_t addr,             /* patched address (32 bits) */
    uint32_t old_value,         /* old value before patching (if it differs, the patch will fail) */
    uint32_t new_value,         /* new value */
    const char* description     /* what does this patch do? example: "raw_rec: slowdown dialog timers" */
                                /* note: you must provide storage for the description string */
                                /* a string literal will do; a local variable where you sprintf will not work */
);

/* a more complex patch (e.g. for 16-bit values, or for flipping only some bits) */
int patch_memory_ex(
    uintptr_t addr,             /* patched address (up to 32 bits) */
    uint32_t check_mask,        /* before patching: what bits to check to make sure we are patching the right thing */
    uint32_t check_value,       /* before patching: expected value of the checked bits */
    uint32_t patch_mask,        /* what bits to patch (up to 32) */
    uint32_t patch_scaling,     /* scaling factor for the old value (0x10000 = 1.0; 0 discards the old value, obviously) */
    uint32_t patch_offset,      /* offset added after scaling (this becomes new_value when scaling factor is 0, also obviously) */
    const char* description     /* what does this patch do? example: "mini_iso: patch CMOS[4] to reduce shadow noise" */
);

/* A patch described by mask, scaling and offset is applied as follows (imagine a masked saxpy):
 *      uint32_t old = backup & mask;
 *      uint32_t new = (uint64_t) old * scaling / 0x10000 + offset;
 *      return new & p->patch_mask;
 *
 * Therefore, a simple 32-bit patch has mask=0xFFFFFFFF, scaling=0 and offset=new_value.
 * This trick covers a lot of cases with a minimal description (without too much increase in complexity).
 */

/* patch a linear array of values (all altered in the same way, as described by mask, scaling and offset) */
int patch_memory_array(
    uintptr_t addr,             /* first patched address (up to 32 bits) */
    int num_items,              /* how many items do we have in the array? */
    int item_size,              /* how many bytes until the second patched address? */
    uint32_t check_mask,        /* before patching: what bits to check to make sure we are patching the right thing */
    uint32_t check_value,       /* before patching: expected value of the checked bits */
    uint32_t patch_mask,        /* what bits to patch (up to 32) */
    uint32_t patch_scaling,     /* scaling factor for the old value (0x10000 = 1.0; 0 discards the old value, obviously) */
    uint32_t patch_offset,      /* offset added after scaling (this becomes new_value when scaling factor is 0, also obviously) */
    uint32_t* backup_storage,   /* must be an array with "num_items" items */
    const char* description     /* what does this patch do? example: "dual_iso: patch CMOS[0] gains" */
);

/* patch a matrix of values (all altered in the same way, as described by mask, scaling and offset) */
int patch_memory_matrix(
    uintptr_t addr,             /* first patched address (up to 32 bits) */
    int num_columns,            /* how many columns do we have in the matrix? */
    int col_size,               /* how many bytes until the second column? */
    int num_rows,               /* how many rows do we have in the matrix? */
    int row_size,               /* how many bytes until the second column? */
    uint32_t check_mask,        /* before patching: what bits to check to make sure we are patching the right thing */
    uint32_t check_value,       /* before patching: expected value of the checked bits */
    uint32_t patch_mask,        /* what bits to patch (up to 32) */
    uint32_t patch_scaling,     /* scaling factor for the old value (0x10000 = 1.0; 0 discards the old value, obviously) */
    uint32_t patch_offset,      /* offset added after scaling (this becomes new_value when scaling factor is 0, also obviously) */
    uint32_t* backup_storage,   /* old values; must be a uint32_t[num_items] array; it can be 0 for a 1x1 matrix => internal storage */
    const char* description     /* what does this patch do? example: "mini_iso: scale ADTG gains" */
);

/* undo the patching done by one of the above calls */
int unpatch_memory(uintptr_t addr);

/* patch a ENGIO register in a FFFFFFFF-terminated list */
/* this will also prevent Canon code from changing that register to some other value (*) */
/* (*) this will only work for Canon code that looks up the register in a list, sets the value if found, and does no error checking */
int patch_engio_list(uint32_t * engio_list, uint32_t patched_register, uint32_t patched_value, const char * description);
int unpatch_engio_list(uint32_t * engio_list, uint32_t patched_register);

/******************************
 * Instruction (code) patches *
 ******************************/

/* patch an executable instruction (will clear the instruction cache) */
/* same arguments as patch_memory */
int patch_instruction(
    uintptr_t addr,
    uint32_t old_value,
    uint32_t new_value,
    const char* description
);

/* to undo, use unpatch_memory(addr) */


/*****************
 * Logging hooks *
 *****************/

/* 
 * Hook a custom logging function in the middle of some ASM code
 * similar to GDB hooks, but lighter:
 * - patches only a single address (slightly lower chances of collision)
 * - does not patch anything when the hook is triggered (self-modifying code runs only once, when set up => faster and less stuff that can break)
 * - uses less black magic (easy to understand by ASM noobs like me)
 * - hooking on instructions that do relative addressing is not fully supported; LDR Rn, [PC, #off] is fine (relocated)
 * - regs contain R0-R12 and LR (be careful)
 * - first 4 args of the inspected function are in regs[0] ... regs[3]
 * - next args are in stack[0], stack[1] and so on
 * - pc is the address where we installed the hook
 * - orig_instr is just for sanity checking
 * 
 * credits: Maqs
 */
typedef void (*patch_hook_function_cbr)(uint32_t* regs, uint32_t* stack, uint32_t pc);

/* to be called only from a patch_hook_function_cbr */
#define PATCH_HOOK_CALLER() (regs[13]-4)    /* regs[13] contains LR, not SP */

int patch_hook_function(uintptr_t addr, uint32_t orig_instr, patch_hook_function_cbr logging_function, const char * description);

/* to undo, use unpatch_memory(addr) */

/* cache sync helper */
int _patch_sync_caches(int also_data);

#endif
