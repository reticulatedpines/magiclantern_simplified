#ifndef _dryos_rpc_h_
#define _dryos_rpc_h_

#if defined(CONFIG_DIGIC_78X) && defined(CONFIG_DUAL_CORE)
// Dual core (which currently are only D78 and X) have a mechanism
// to do RPC from core to core.  Internally this uses SGI,
// interrupt 0xc, though it's not necessary to expose this to 
// ML code to use RPC.

struct RPC_args
{
    void (*RPC_func)(void *);
    void *RPC_arg; // argument to be passed to above func
};

extern struct semaphore *RPC_sem;

// The DryOS _request_RPC() will call the passed function forever,
// if you don't manually clear the request.  We wrap this via
// do_RPC() to avoid mistakes.  Consequently, the _request_RPC() stub
// shouldn't be used.
int request_RPC(struct RPC_args *args);

#endif // CONFIG_DIGIC_78X && CONFIG_DUAL_CORE

#endif //_dryos_rpc_h_
