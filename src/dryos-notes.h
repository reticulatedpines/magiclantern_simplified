/* reverse engineering notes, moved from dryos.h (not used in ML code) */

/** Panic and abort the camera */
extern void __attribute__((noreturn))
DryosPanic(
        uint32_t                arg0,
        uint32_t                arg1
);

/** Memory allocation */
struct dryos_meminfo
{
        void * next; // off_0x00;
        void * prev; // off_0x04;
        uint32_t size; //!< Allocated size or if bit 1 is set already free
};
SIZE_CHECK_STRUCT( dryos_meminfo, 0xC );

/** Battery management */
extern void GUI_SetErrBattery( unsigned ok );
extern void StopErrBatteryApp( void );
extern void * err_battery_ptr;

/** Power management */
extern void powersave_disable( void );
extern void powersave_enable( void );

#define EM_ALLOW 1
#define EM_PROHIBIT 2
extern void prop_request_icu_auto_poweroff( int mode );


extern int
oneshot_timer(
        uint32_t                msec,
        void                    (*handler_if_expired)(void*),
        void                    (*handler)(void*),
        void *                  arg
);


/** Official Canon sound device task.
 * \internal
 */
extern void sound_dev_task(void);

extern void audio_set_alc_on(void);
extern void audio_set_alc_off(void);
extern void audio_set_filter_off(void);
extern void audio_set_windcut(int);
extern void audio_set_sampling_param(int, int, int);
extern void audio_set_volume_in(int,int);
extern void audio_start_asif_observer(void);
extern void audio_level_task(void);
extern void audio_interval_unlock(void*);

extern FILE *
FIO_OpenFile(
        const char *            name
#ifdef CONFIG_VXWORKS
		,
		int flags,
		int mode
#endif
);

extern ssize_t
FR_SyncReadFile(
        const char *            filename,
        size_t                  offset,
        size_t                  len,
        void *                  address,
        size_t                  mem_offset
);

extern void
FIO_CloseSync(
        void *                  file
);



extern void
write_debug_file(
        const char *            name,
        const void *            buf,
        size_t                  len
);

struct lvram_info
{
        uint32_t                off_0x00;
        uint32_t                off_0x04;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                width;          // off_0x18;
        uint32_t                height;         // off_0x1c;
        uint32_t                off_0x20;
        uint32_t                off_0x24;
        uint32_t                off_0x28;
        uint32_t                off_0x2c;
        uint32_t                off_0x30;
        uint32_t                off_0x34;
        uint32_t                off_0x38;
        uint32_t                off_0x3c;
        uint32_t                off_0x40;
        uint32_t                off_0x44;
        uint32_t                off_0x48;
        uint32_t                off_0x4c;
        uint32_t                off_0x50;
        uint32_t                off_0x54;
        uint32_t                off_0x58;
        uint32_t                off_0x5c;
        uint32_t                off_0x60;
};
SIZE_CHECK_STRUCT( lvram_info, 0x64 );
extern struct lvram_info lvram_info;


/* Copies lvram info from 0x2f33c */
extern void
copy_lvram_info(
        struct lvram_info *     dest
);

struct image_play_struct
{
        uint32_t                off_0x00;
        uint16_t                off_0x04; // sharpness?
        uint16_t                off_0x06;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                copy_vram_mode;                 // off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                image_player_effective;         // off_0x1c;
        uint32_t                vram_num;                       // off_0x20;
        uint32_t                work_image_pataion;             // off_0x24 ?;
        uint32_t                visible_image_vram_offset_x;    // off_0x28;
        uint32_t                visible_image_vram_offset_y;    // off_0x2c;
        uint32_t                work_image_id;                  // off_0x30;
        uint32_t                off_0x34;
        uint32_t                image_aspect;                   // off_0x38;
        uint32_t                off_0x3c;
        uint32_t                off_0x40;
        uint32_t                off_0x44;
        uint32_t                sharpness_rate;                 // off_0x48;
        uint32_t                off_0x4c;
        uint32_t                off_0x50;       // passed to gui_change_something
        uint32_t                off_0x54;
        struct semaphore *      sem;                            // off_0x58;
        uint32_t                off_0x5c;
        uint32_t                image_vram;                     // off_0x60;
        uint32_t                off_0x64;
        uint32_t                rectangle_copy;                 // off_0x68;
        uint32_t                image_play_driver_handler;      // off_0x6c;
        uint32_t                off_0x70;
        uint32_t                image_vram_complete_callback;   // off_0x74;
        uint32_t                off_0x78;
        uint32_t                work_image_width;               // off_0x7c;
        uint32_t                work_image_height;              // off_0x80;
        uint32_t                off_0x84;
        uint32_t                off_0x88;
        uint32_t                off_0x8c;
        uint32_t                off_0x90;
        uint32_t                off_0x94;
        uint32_t                off_0x98;
        uint32_t                off_0x9c;
};

extern struct image_play_struct image_play_struct;


/** The top-level Liveview object.
 * 0x2670 bytes; it is huge!
 */
struct liveview_mgr
{
        const char *            type;           // "LiveViewMgr"
        struct task *           task;           // off 0x04
        uint32_t                off_0x08;
        struct state_object *   lv_state;       // off 0x0c
};

extern struct liveview_mgr * liveview_mgr;

struct lv_struct
{
        uint32_t                off_0x00;
        uint32_t                off_0x04;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        struct state_object *   lv_state;       // off 0x20
        struct state_object *   lv_rec_state;   // off 0x24
};


/** Main menu tab functions */
extern int main_tab_dialog_id;
extern void main_tab_header_dialog( void );
extern void StopMnMainTabHeaderApp( void );
extern void StartMnMainRec1App( void );
extern void StartMnMainRec2App( void );
extern void StartMnMainPlay1App( void );
extern void StartMnMainPlay2App( void );
extern void StartMnMainSetup1App( void );
extern void StartMnMainSetup2App( void );
extern void StartMnMainSetup3App( void );
extern void StartMnMainCustomFuncApp( void );
extern void StartMnMainMyMenuApp( void );


/** Hidden menus */
extern void StartFactoryMenuApp( void );
extern void StartMnStudioSetupMenuApp( void );


extern size_t
strftime(
        char *                  buf,
        size_t                  maxsize,
        const char *            fmt,
        const struct tm *       timeptr
);



// others
extern int abs( int number );

// get OS constants
extern const char* get_card_drive( void );

extern int _dummy_variable;

/*********************************************************************
 *
 *  Controller struct, present in Digic5 cameras like the 5d3 and 6D.
 *
 *  Seems to be highly related to VRAM buffers, probably necessary
 *  to understand this before we explore resizing / creating our
 *  own buffers. EVF_STRUCT is at 0x76D18 in the 6D.112 firmware.
 *
 *********************************************************************/

/*  Controllers created in 6D.112:
 *
 *       Name                   Address           Struct Size       Pointer
 *  --------------------------------------------------------------------------
 *      ENCODE_CON              0x1F9D4             0x1420          0x7742C
 *      SsDevelopStage          0x20980             0x10            0x77458
 *      VramStage               0x21838             0x80            0x7747C
 *      AEWB_Controller         0x24EA4             0x1CC           unknown  <-- idk, it just returns the pointer caller (but, has no caller)
 *      AF_Controller           0x321C8             0x18C           unknown
 *      VRAM_CON                0x3AC74             0xC8            unknown
 *      BUF_CON                 0x3BED0             0x1C            EVF_STRUCT->off_0x14
 *      SSDEV_CON               0x411D8             0x220           unknown
 *      VRAM_CON                0x42048             0x2B0           unknown
 *      Color_Controller        0xFF24E3B8          0xC80           unkonwn
 *      FLICK_CON               0xFF35AB00          0x10            unknown
 *      REMOTE_CON              0xFF362F3C          0x2754          0x7A994
 *      AFAE_Controller         0xFF4267E8          0xF0            unknown
 *      ObInteg_Controller      0xFF426B48          0x6C            unknown
 *      SceneJudge_Controller   0xFF4324F0          0xC             unknown
 *
 */

struct Controller
{
    const char *                    name;                   //~ off_0x00    Name of controller.
    int                             taskclass_ptr;          //~ off_0x04    Pointer to taskclass struct.
    int                             stateobj_ptr;           //~ off_0x08    Pointer to state object, if no stateobj this is set to 1.
    int                             off_0x0c;               //~ unknown
    int                             jobqueue_ptr;           //~ off_0x10    Pointer to JobQueue.
};
