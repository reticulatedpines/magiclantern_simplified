#include <stddef.h>

#include "config-defines.h"
#include "dryos.h"
#include "dryos_rpc.h"

#if defined(CONFIG_DIGIC_78X) && defined(CONFIG_RPC)

// This sem guards against having multiple RPC
// requests in flight at once.
//
// In order to ensure clear_RPC_request() gets called,
// we wrap the real target func in do_RPC().  The args
// to that are passed in, and setup by cpu0, the code runs
// on cpu1.
//
// We copy the args and work from those to ensure the args
// remain valid for the lifetime of the RPC request.
//
// This means the caller of request_RPC() doesn't need to
// care about param lifetime or semaphores (other than
// checking the return value from request_RPC()).
struct semaphore *RPC_sem; // initialised in boot_pre_init_task()

// this gets run by the other cpu
static void do_RPC(void *args)
{
    extern int clear_RPC_request(void);
    struct RPC_args *a = (struct RPC_args *)args;
    a->RPC_func(a->RPC_arg);
    clear_RPC_request();
    a->RPC_func = 0;
    a->RPC_arg = 0;
    // request complete, cpu1 releases sem
    give_semaphore(RPC_sem);
}

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

    // we can only have one request in flight, cpu0 takes sem
    int sem_res = take_semaphore(RPC_sem, 0);
    if (sem_res != 0)
        return -1;

    // storage for RPC params must remain valid until cpu1 completes request,
    // don't trust the caller to do this.
    static struct RPC_args RPC_args = {0};
    RPC_args.RPC_func = args->RPC_func;
    RPC_args.RPC_arg = args->RPC_arg;

    int status = _request_RPC(do_RPC, (void *)&RPC_args);
    return status;
}

#endif // CONFIG_DIGIC_78X && CONFIG_RPC
