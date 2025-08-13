#ifndef _hooks_thumb_h_
#define _hooks_thumb_h_

#include <stdint.h>

// D78X cams have no expected signature; we must do more work in the function
// to behave safely, but we have more flexibility.  We can't jump very far in
// one instruction in Thumb, so it's harder to be as generic as the ARM hook code.
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_CMOS_write_200D(void);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_CMOS_write_6D2(void);

#endif
