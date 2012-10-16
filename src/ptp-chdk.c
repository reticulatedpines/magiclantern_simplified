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


extern uint32_t gdb_recv_buffer_length;
extern uint32_t gdb_send_buffer_length;
extern uint8_t gdb_recv_buffer[];
extern uint8_t gdb_send_buffer[];
extern void gdb_recv_callback(uint32_t);
extern void gdb_send_callback();

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

    // handle command
    switch ( param1 )
    {

        case PTP_CHDK_Version:
            msg.param_count = 2;
            msg.param[0] = PTP_CHDK_VERSION_MAJOR;
            msg.param[1] = PTP_CHDK_VERSION_MINOR;
            break;



        case PTP_CHDK_GetMemory:
            {
                uint32_t pos = 0;
                uint32_t address = param2;
                uint32_t length = param3;
                uint8_t *buf = NULL;

                if ( length == 0 )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                buf = AllocateMemory( length );

                if ( !buf )
                {
                    msg.id = PTP_RC_GeneralError;
                    break;
                }

                while ( pos < length )
                {
                    if ( (length - pos) >= 4 )
                    {
                        ((uint32_t*)buf)[pos / 4] = *((uint32_t*)(address + pos));
                        pos += 4;
                    }
                    else
                    {
                        buf[pos] = *((uint8_t*)(address + pos));
                        pos++;
                    }
                }

                if ( !send_ptp_data(context, (char *) buf, length) )
                {
                    msg.id = PTP_RC_GeneralError;
                }

                FreeMemory(buf);
            }
            break;

        case PTP_CHDK_SetMemory:
            if ( param2 == 0 || param3 < 1 ) // null pointer or invalid size?
            {
                msg.id = PTP_RC_GeneralError;
                break;
            }

            context->get_data_size(context->handle); // XXX required call before receiving
            if ( !recv_ptp_data(context,(char *) param2,param3) )
            {
                msg.id = PTP_RC_GeneralError;
            }
            break;

#ifdef GDBSTUB
        case PTP_CHDK_GDBStub_Download:
            {
                uint32_t length = param2;
                if (param2 == 0) // null pointer or invalid size?
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
                uint32_t length = param2;
                
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
                    //bmp_printf(FONT_LARGE, 0, 30, gdb_send_buffer);
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
                buf = AllocateMemory(size);

                if ( !recv_ptp_data(context, (char *)buf, size) )
                {
                    msg.id = PTP_RC_GeneralError;
                }
                else
                {
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
                    default:
                        bmp_printf(FONT_LARGE, 0, 0, ">= 5 args not supported");
                        msg.id = PTP_RC_GeneralError;
                        break;
                    }

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
                    FreeMemory(temp_data.str);
                } else if ( temp_data_kind == 2 )
                {
                    //~ lua_close(temp_data.lua_state);
                }
                temp_data_kind = 0;

                temp_data_extra = context->get_data_size(context->handle);

                temp_data.str = (char *) AllocateMemory(temp_data_extra);
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
                    FreeMemory(temp_data.str);
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

                fn = (char *) AllocateMemory(fn_len+1);
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
                    FreeMemory(fn);
                    break;
                }
                FreeMemory(fn);

                buf = (char *) AllocateMemory(BUF_SIZE);
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

                FreeMemory(buf);
                break;
            }

        case PTP_CHDK_DownloadFile:
            {
                FILE *f;
                int tmp,t,r;
                unsigned s;

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

                FreeMemory(temp_data.str);
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

                char buf[BUF_SIZE+32];
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


