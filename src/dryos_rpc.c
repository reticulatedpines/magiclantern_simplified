#include <stddef.h>

#include "config-defines.h"
#include "dryos.h"
#include "dryos_rpc.h"

#if defined(CONFIG_DIGIC_78X) && defined(CONFIG_RPC)

// This sem guards against having multiple RPC
// requests in flight at once.  Technically, this
// is redundant, on the cams I've checked.  DryOS
// disables interrupts and uses locking to ensure
// only one RPC request occurs at once (and the data
// is kept in a global so it simply wouldn't work).
//
// However, this is conceptually the right thing to
// do (we don't know how a given cam will behave).
//
// In order to ensure clear_RPC_request() gets called,
// we wrap the real target func in do_RPC().  The args
// to that are passed in, and setup by cpu0, the code runs
// on cpu1.  The args must remain valid for the
// lifetime of the RPC request.
// The semaphore also ensures that;
// use it to guard accesses to RPC_args.
struct semaphore *RPC_sem; // initialised in boot_pre_init_task()
struct RPC_args RPC_args = {0};

// The DryOS _request_RPC() will call the passed function forever,
// if you don't manually clear the request.  We wrap this via
// do_RPC() to avoid mistakes.  Consequently, the _request_RPC() stub
// shouldn't be used.
//
// Note the param must fit in a single reg, _request_RPC() can only be used
// to call funcs taking 1 param, since the func pointer is called via "blx r1".
// Notably, ARM calling convention will pack small structs into r0-r3.
int request_RPC(struct RPC_args *args)
{
    extern int _request_RPC(void (*f)(void *), void *o);
    int status = _request_RPC(do_RPC, (void *)args);
    return status;
}

void do_RPC(void *args)
{
    extern int clear_RPC_request(void);
    struct RPC_args *a = (struct RPC_args *)args;
    a->RPC_func(a->RPC_arg);
    clear_RPC_request();
    RPC_args.RPC_func = 0;
    RPC_args.RPC_arg = 0;
    give_semaphore(RPC_sem);
}

#endif // CONFIG_DIGIC_78X && CONFIG_RPC
