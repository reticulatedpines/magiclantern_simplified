#ifndef _ml_rpc_h_
#define _ml_rpc_h_

#define ML_RPC_PING           (0x80000000)
#define ML_RPC_PING_REPLY     (0x80000001)
#define ML_RPC_CACHE_HACK     (0x80000002)
#define ML_RPC_CACHE_HACK_DEL (0x80000003)
#define ML_RPC_CALL           (0x80000004)
#define ML_RPC_ENGIO_WRITE    (0x80000005)
#define ML_RPC_ENGIO_READ     (0x80000006)
#define ML_RPC_READ           (0x80000007)
#define ML_RPC_READ_REPLY     (0x80000008)
#define ML_RPC_OK             (0xFEEFEE00)
#define ML_RPC_ERROR          (0xFEEFEEEE)


#define ML_RPC_ID_SLAVE      (0x40FE)
#define ML_RPC_ID_MASTER     (0x40FF)
#define ML_RPC_ID_VIGNETTING (0x40FD)

typedef struct
{
    uint32_t message;
    uint32_t parm1;
    uint32_t parm2;
    uint32_t parm3;
    uint32_t wait;
} ml_rpc_request_t;


uint32_t ml_rpc_send(uint32_t command, uint32_t parm1, uint32_t parm2, uint32_t parm3, uint32_t wait);
uint32_t ml_rpc_send_recv(uint32_t command, uint32_t *parm1, uint32_t *parm2, uint32_t *parm3, uint32_t wait);
uint32_t ml_rpc_call(uint32_t address, uint32_t arg0, uint32_t arg1);
uint32_t ml_rpc_readmem(uint32_t address, uint32_t length, uint8_t *buffer);
uint32_t ml_rpc_send_vignetting(uint32_t *buffer);

#endif