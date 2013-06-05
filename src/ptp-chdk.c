/** \file
* PTP handlers to extend Magic Lantern to the USB port.
*
* These handlers are registered to allow Magic Lantern to interact with
* a PTP client on the USB port.
*/

#include "dryos.h"
#include "ptp.h"
#include "ptp-chdk.h"
#include "tasks.h"
#include "menu.h"
#include "bmp.h"
#include "property.h"
#include "lens.h"

/* if this is defined, any memory read will read processor cache. hacker's toolbox. keep fingers off. */
//#define PTP_CACHE_ACCESS

/* access master processor memories instead of local (slave) */
#if defined(CONFIG_7D)
//#define PTP_7D_MASTER_ACCESS
#endif

#if defined(CONFIG_GDBSTUB)
extern uint32_t gdb_recv_buffer_length;
extern uint32_t gdb_send_buffer_length;
extern uint8_t gdb_recv_buffer[];
extern uint8_t gdb_send_buffer[];
extern void gdb_recv_callback(uint32_t);
extern void gdb_send_callback();
#endif


#if defined(PTP_7D_MASTER_ACCESS)
/* these can read any address in master, but if it is RAM, it is always in uncacheable. 
   this means, we can not read the first 8k at 0x00000000 as this is cacheable only and not mirrored at 0x40000000
*/
uint32_t BulkOutIPCTransfer(int type, uint8_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t, uint32_t, uint32_t), uint32_t cb_parm);
uint32_t BulkInIPCTransfer(int type, uint8_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t, uint32_t, uint32_t), uint32_t cb_parm);

void ptp_bulk_cb(uint32_t *parm, uint32_t address, uint32_t length)
{
    *parm = 0;
}
#endif


/* want to access cache memory? */
#if defined(PTP_CACHE_ACCESS)
#include "cache_hacks.h"

uint32_t ptp_get_cache(uint32_t segment, uint32_t index, uint32_t word, uint32_t type, uint32_t want_tag)
{
    uint32_t tag = 0;
    uint32_t data = 0;

    cache_get_content(segment, index, word, type, &tag, &data);
    
    if(want_tag)
    {
        return tag;
    }
    else
    {
        return data;
    }
}
#endif



PTP_HANDLER( PTP_OC_CHDK, 0 )
{
    struct ptp_msg msg = 
    {
        .id          = PTP_RC_OK,
        .session     = session,
        .transaction = transaction,
        .param_count = 4,
        .param       = { 1, 2, 0xdeadbeef, 3 },
    };
    
    static int temp_data_kind = 0; // 0: nothing, 1: ascii string, 2: lua object
    static int temp_data_extra; // size (ascii string) or type (lua object)

    static union {
        char *str;
    } temp_data;
    
    uint32_t address = param2;
    uint32_t length = param3;
    uint8_t *buf = NULL;
#if defined(PTP_7D_MASTER_ACCESS)
    uint32_t ret = 0;
    uint32_t errcount = 0;
#endif

    // handle command
    switch ( param1 )
    {

        case PTP_CHDK_Version:
            msg.param_count = 2;
            msg.param[0] = PTP_CHDK_VERSION_MAJOR;
            msg.param[1] = PTP_CHDK_VERSION_MINOR;
    #if defined(CONFIG_DIGIC_V) && !defined(CONFIG_5D3)
            //~ unlock camera UI once ptpcam connects
            ptpPropSetUILock(0, 2);
    #endif
            break;



        case PTP_CHDK_GetMemory:
            {
                uint32_t pos = 0;

                if ( length == 0 )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                buf = alloc_dma_memory(length);

                if ( !buf )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }
                
                memset(buf, 0xEE, length);

#if defined(PTP_7D_MASTER_ACCESS)
                volatile uint32_t wait = 1;
                while(ret = BulkInIPCTransfer(0, buf, length, address, &ptp_bulk_cb, &wait))
                {
                    errcount++;
                    bmp_printf(FONT_LARGE, 0, 0, "Bulk fail err: %X count: %X", ret, errcount);

                    msleep(100);
                }
                
                while(wait)
                {
                    msleep(100);
                }
                
#else
                while ( pos < length )
                {
                    if ( (length - pos) >= 4 )
                    {
#if defined(PTP_CACHE_ACCESS)
                        uint32_t index = 0;
                        uint32_t ptp_get_cache(uint32_t segment, uint32_t index, uint32_t word, uint32_t type, uint32_t want_tag);
                        
                        uint32_t tag = ptp_get_cache((address >> 16) & 3, (address & 0x7E0) >> 5, (address & 0x1C) >> 2, 0, 1);
                        
                        if((tag & 0x10) == 0)
                        {
                            ((uint32_t*)buf)[pos/4] = 0xEEEEEEEE;
                        }
                        else
                        {
                            ((uint32_t*)buf)[pos/4] = ptp_get_cache((address >> 16) & 3, (address & 0x7E0) >> 5, (address & 0x1C) >> 2, 0, (address >> 20) & 1);
                        }
#else
                        ((uint32_t*)buf)[pos / 4] = *((uint32_t*)(address));
#endif
                        address += 4;
                        pos += 4;
                    }
                    else
                    {
                        buf[pos] = *((uint8_t*)(address));
                        pos++;
                        address++;
                    }
                }
#endif

                if ( !send_ptp_data(context, (char *) buf, length) )
                {
                    msg.id = PTP_RC_GeneralError;
                }

                free_dma_memory(buf);
            }
            break;

        case PTP_CHDK_SetMemory:
            
            if ( length < 1 ) // invalid size?
            {
                msg.id = PTP_RC_GeneralError;
                break;
            }
            
#if defined(PTP_7D_MASTER_ACCESS)
            length = context->get_data_size(context->handle);
            buf = alloc_dma_memory( length );

            if ( !buf )
            {
                msg.id = PTP_RC_GeneralError;
                break;
            }
            
            if ( !recv_ptp_data(context,buf,length) )
            {
                msg.id = PTP_RC_GeneralError;
                free_dma_memory(buf);
                break;
            }
            
            volatile uint32_t wait = 1;
            
            while(ret = BulkOutIPCTransfer(0, buf, length, address, &ptp_bulk_cb, &wait))
            {
                errcount++;
                bmp_printf(FONT_LARGE, 0, 0, "Bulk fail err: %X count: %X", ret, errcount);

                msleep(100);
            }
            
            while(wait)
            {
                msleep(100);
            }
            free_dma_memory(buf);
            
#else      
            context->get_data_size(context->handle); // XXX required call before receiving
            
            if ( !recv_ptp_data(context,(char *) param2,param3) )
            {
                msg.id = PTP_RC_GeneralError;
            }
#endif
            break;

#ifdef CONFIG_GDBSTUB // Automatically defined by Make if CONFIG_GDB = y
        case PTP_CHDK_GDBStub_Download:
            {
                if (length == 0) // invalid size?
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }
                
                /* buffer not processed yet */
                while(gdb_recv_buffer_length != 0)
                {
                    msleep(1);
                }

                context->get_data_size(context->handle); // XXX required call before receiving
                if ( !recv_ptp_data(context, (char*)gdb_recv_buffer, length) )
                {
                    msg.id = PTP_RC_GeneralError;
                }
                
                /* mark as filled */              
                gdb_recv_callback(length);
            }
            break;       
            
        case PTP_CHDK_GDBStub_Upload:
            {
                /* buffer not filled yet */
                if(gdb_send_buffer_length == 0)
                {
                    char dummy = 0;
                    
                    if (!send_ptp_data(context, &dummy, 1) )
                    {
                        msg.id = PTP_RC_GeneralError;
                    }
                }
                else
                {
                    if (!send_ptp_data(context, (char*)gdb_send_buffer, gdb_send_buffer_length) )
                    {
                        msg.id = PTP_RC_GeneralError;
                    }
                
                    /* mark as free again */
                    gdb_send_callback();
                }
            }
            break;

#endif

        case PTP_CHDK_CallFunction:
            {
                uint32_t ret = 0;
                uint32_t size = 0;
                uint32_t *buf = NULL;

                size = context->get_data_size(context->handle);
                buf = alloc_dma_memory(size);

                if ( !recv_ptp_data(context, (char *)buf, size) )
                {
                    msg.id = PTP_RC_GeneralError;
                }
                else
                {
                    #if defined(PTP_7D_MASTER_ACCESS)
                    bmp_printf(FONT_LARGE, 0, 0, "BL MASTER 0x%08X", buf[0]);

                    switch((size/4) - 1)
                    {
                    case 0:
                        ret = ml_rpc_call(buf[0], 0, 0);
                        break;
                    case 1:
                        ret = ml_rpc_call(buf[0], buf[1], 0);
                        break;
                    case 2:
                        ret = ml_rpc_call(buf[0], buf[1], buf[2]);
                        break;
                    default:
                        bmp_printf(FONT_LARGE, 0, 0, ">= 3 args not supported");
                        msg.id = PTP_RC_GeneralError;
                        break;
                    }
                    #else
                    bmp_printf(FONT_LARGE, 0, 0, "BL 0x%08X", buf[0]);

                    switch((size/4) - 1)
                    {
                    case 0:
                        ret = ((uint32_t (*)())buf[0])();
                        break;
                    case 1:
                        ret = ((uint32_t (*)(int))buf[0])(buf[1]);
                        break;
                    case 2:
                        ret = ((uint32_t (*)(int,int))buf[0])(buf[1],buf[2]);
                        break;
                    case 3:
                        ret = ((uint32_t (*)(int,int,int))buf[0])(buf[1],buf[2],buf[3]);
                        break;
                    case 4:
                        ret = ((uint32_t (*)(int,int,int,int))buf[0])(buf[1],buf[2],buf[3],buf[4]);
                        break;
                    case 5:
                        ret = ((uint32_t (*)(int,int,int,int,int))buf[0])(buf[1],buf[2],buf[3],buf[4],buf[5]);
                        break;
                    case 6:
                        ret = ((uint32_t (*)(int,int,int,int,int,int))buf[0])(buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                        break;
                    default:
                        bmp_printf(FONT_LARGE, 0, 0, ">= 5 args not supported");
                        msg.id = PTP_RC_GeneralError;
                        break;
                    }
                    #endif
                    msg.param_count = 1;
                    msg.param[0] = ret;
                }
            }
            break;

        case PTP_CHDK_TempData:
            if ( param2 & PTP_CHDK_TD_DOWNLOAD )
            {
                const char *s = NULL;
                size_t l = 0;

                if ( temp_data_kind == 0 )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                if ( temp_data_kind == 1 )
                {
                    s = temp_data.str;
                    l = temp_data_extra;
                } 
                else
                {
                    s = 0;
                    //~ s = lua_tolstring(get_lua_thread(temp_data.lua_state),1,&l);
                }

                if ( !send_ptp_data(context,s,l) )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }
            } 
            else if ( !(param2 & PTP_CHDK_TD_CLEAR) ) 
            {
                if ( temp_data_kind == 1 )
                {
                    free_dma_memory(temp_data.str);
                } else if ( temp_data_kind == 2 )
                {
                    //~ lua_close(temp_data.lua_state);
                }
                temp_data_kind = 0;

                temp_data_extra = context->get_data_size(context->handle);

                temp_data.str = (char *) alloc_dma_memory(temp_data_extra);
                if ( temp_data.str == NULL )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                if ( !recv_ptp_data(context,temp_data.str,temp_data_extra) )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }
                temp_data_kind = 1;
            }
            if ( param2 & PTP_CHDK_TD_CLEAR )
            {
                if ( temp_data_kind == 1 )
                {
                    free_dma_memory(temp_data.str);
                }
                else if ( temp_data_kind == 2 )
                {
                    //~ lua_close(temp_data.lua_state);
                }
                temp_data_kind = 0;
            }
            break;

        case PTP_CHDK_UploadFile:
            {
                FILE *f;
                int s,fn_len;
                char *buf, *fn;

                s = context->get_data_size(context->handle);

                recv_ptp_data(context,(char *) &fn_len,4);
                s -= 4;

                fn = (char *) alloc_dma_memory(fn_len+1);
                if ( fn == NULL )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }
                fn[fn_len] = '\0';

                recv_ptp_data(context,fn,fn_len);
                s -= fn_len;

                bmp_printf(FONT_LARGE, 0, 0, "UL '%s' %db", fn, s);

                FIO_RemoveFile(fn);
                f = FIO_CreateFile(fn);
                if ( f == NULL )
                {
                    msg.id = PTP_RC_GeneralError;
                    free_dma_memory(fn);
                    break;
                }
                free_dma_memory(fn);

                buf = (char *) alloc_dma_memory(BUF_SIZE);
                if ( buf == NULL )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }
                while ( s > 0 )
                {
                    if ( s >= BUF_SIZE )
                    {
                        recv_ptp_data(context,buf,BUF_SIZE);
                        FIO_WriteFile(f, UNCACHEABLE(buf), BUF_SIZE);
                        s -= BUF_SIZE;
                    } else {
                        recv_ptp_data(context,buf,s);
                        FIO_WriteFile(f, UNCACHEABLE(buf), s);
                        s = 0;
                    }
                }

                FIO_CloseFile(f);

                free_dma_memory(buf);
                break;
            }

        case PTP_CHDK_DownloadFile:
            {
                FILE *f;
                int tmp,t,r;
                uint32_t s;

                bmp_printf(FONT_LARGE, 0, 0, "DL request");

                if ( temp_data_kind != 1 )
                {
                    bmp_printf(FONT_LARGE, 0, 0, "DL kind err %d", temp_data_kind);
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                char fn[101];
                if (temp_data_extra > 100)
                {
                    bmp_printf(FONT_LARGE, 0, 0, "DL extra err %d", temp_data_extra);
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                memcpy(fn,temp_data.str,temp_data_extra);
                fn[temp_data_extra] = '\0';

                free_dma_memory(temp_data.str);
                temp_data_kind = 0;

                if( FIO_GetFileSize( fn, &s ) != 0 )
                {
                    bmp_printf(FONT_LARGE, 0, 0, "DL '%s' size err", fn);
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                bmp_printf(FONT_LARGE, 0, 0, "DL '%s' %db", fn, s);

                f = FIO_Open(fn, 0);
                if ( f == NULL )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                char *buf;
                buf = (char *) alloc_dma_memory(BUF_SIZE);
                if ( buf == NULL )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }
                tmp = s;
                t = s;
                while ( (r = FIO_ReadFile(f, UNCACHEABLE(buf), (t<BUF_SIZE) ? t : BUF_SIZE)) > 0 )
                {
                    t -= r;
                    // cannot use send_ptp_data here
                    context->send_data(context->handle,buf,r,tmp,0,0,0);
                    tmp = 0;
                }
                FIO_CloseFile(f);
                free_dma_memory(buf);
                // XXX check that we actually read/send s bytes! (t == 0)

                msg.param_count = 1;
                msg.param[0] = s;

                break;
            }
            break;

        case PTP_CHDK_ExecuteScript:
            bmp_printf(FONT_LARGE, 0, 0, "ExecuteScript: not implemented");
            msleep(1000);
            break;

        default:
            msg.id = PTP_RC_ParameterNotSupported;
            break;
    }

    context->send_resp( context->handle, &msg );

    return 0;
}
