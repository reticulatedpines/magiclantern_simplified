#include <stdint.h>

struct edmac_info_D4567
{
    unsigned int off1a;
    unsigned int off1b;
    unsigned int off2a;
    unsigned int off2b;
    unsigned int off3;
    unsigned int xa;
    unsigned int xb;
    unsigned int ya;
    unsigned int yb;
    unsigned int xn;
    unsigned int yn;
};

// D45 cams use patch_hook_function(), which expects a standard
// sig for the hook code
void hook_CreateResLockEntry_70D(uint32_t *regs, uint32_t *stack, uint32_t pc);
void hook_StartEDMAC_70D(uint32_t *regs, uint32_t *stack, uint32_t pc);
void hook_ConnectWriteEDMAC_70D(uint32_t *regs, uint32_t *stack, uint32_t pc);
void hook_ConnectReadEDMAC_70D(uint32_t *regs, uint32_t *stack, uint32_t pc);
void hook_SetEDMAC_70D(uint32_t *regs, uint32_t *stack, uint32_t pc);
//void hook_CreateResLockEntry_70D(uint32_t *regs, uint32_t *stack, uint32_t pc);

void hook_StartEDMAC_5D3(uint32_t *regs, uint32_t *stack, uint32_t pc);
void hook_ConnectWriteEDMAC_5D3(uint32_t *regs, uint32_t *stack, uint32_t pc);
void hook_ConnectReadEDMAC_5D3(uint32_t *regs, uint32_t *stack, uint32_t pc);
void hook_SetEDMAC_5D3(uint32_t *regs, uint32_t *stack, uint32_t pc);

// D78X cams have no expected signature; we must do more work in the function
// to behave safely, but we have more flexibility.  We can't jump very far in
// one instruction in Thumb, so it's harder to be as generic as the ARM hook code.
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_StartEDMAC_200D(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_ConnectWriteEDMAC_200D(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_ConnectReadEDMAC_200D(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_set_addr_200D(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_set_size_200D(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_CreateResLockEntry_200D(void);

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_StartEDMAC_6D2(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_ConnectWriteEDMAC_6D2(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_ConnectReadEDMAC_6D2(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_set_addr_6D2(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_set_size_6D2(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_CreateResLockEntry_6D2(void);
