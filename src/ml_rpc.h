#ifndef _ml_rpc_h_
#define _ml_rpc_h_

#define ML_RPC_PING           (0x80000000)
#define ML_RPC_PING_REPLY     (0x80000001)
#define ML_RPC_CACHE_HACK     (0x80000002)
#define ML_RPC_CACHE_HACK_DEL (0x80000003)
#define ML_RPC_CALL           (0x80000004)
#define ML_RPC_OK             (0xFEEFEE00)
#define ML_RPC_ERROR          (0xFEEFEEEE)


#define ML_RPC_ID_SLAVE      (0x40FE)
#define ML_RPC_ID_MASTER     (0x40FF)

typedef struct
{
    uint32_t message;
    uint32_t parm1;
    uint32_t parm2;
    uint32_t parm3;
} ml_rpc_request_t;


uint32_t ml_rpc_send(uint32_t command, uint32_t parm1, uint32_t parm2, uint32_t parm3, uint32_t wait);
uint32_t ml_rpc_call(uint32_t address, uint32_t arg0, uint32_t arg1);

#endif