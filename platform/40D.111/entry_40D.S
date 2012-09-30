.text
.org 0
.globl _start, start, init_code_run

start:
_start:
        B  copy_and_restart

init_code_run:										// original : sub_FF810054
                LDR     R0, =0xC0400000
                LDR     R1, =0x430005
                STR     R1, [R0,#8]
                MOV     R0, #0xC0000000
                LDR     R1, =0xD9C50000
                STR     R1, [R0,#0x10]
                LDR     R2, =0xC0200000
                MOV     R1, #0x0FFFFFFFF
                STR     R1, [R2,#0x10C]
                STR     R1, [R2,#0xC]
                STR     R1, [R2,#0x1C]
                STR     R1, [R2,#0x2C]
                STR     R1, [R2,#0x3C]
                STR     R1, [R2,#0x4C]
                STR     R1, [R2,#0x5C]
                STR     R1, [R2,#0x6C]
                STR     R1, [R2,#0x7C]
                STR     R1, [R2,#0x8C]
                STR     R1, [R2,#0x9C]
                STR     R1, [R2,#0xAC]
                STR     R1, [R2,#0xBC]
                STR     R1, [R2,#0xCC]
                STR     R1, [R2,#0xDC]
                STR     R1, [R2,#0xEC]
                STR     R1, [R2,#0xFC]
                LDR     SP, =0x1900
                MOV     R11, #0
                LDR     PC, =my_romStart


my_romStart:											// original : sub_FF8100DC
                MOV     R12, SP
                STMFD   SP!, {R4,R11,R12,LR,PC}
                SUB     R11, R12, #4
                MOV     R4, R0
                LDR     R3, =0x25530
                SUB     R2, R3, #0x1900
                MOV     R3, R2,LSR#2
                LDR     R0, =0xFFD89CC0
                MOV     R1, #0x1900
                MOV     R2, R3
                BL      unknown_cache
                MOV     R0, R4
                BL      my_usrInit
                LDMDB   R11, {R4,R11,SP,PC}


my_usrInit:												// original : sub_FF810FC8
                MOV     R12, SP
                STMFD   SP!, {R11,R12,LR,PC}
                SUB     R11, R12, #4
                SUB     SP, SP, #4
                STR     R0, [R11,#-0x10]
                LDR     R0, [R11,#-0x10]
                BL      sysStart
                MOV     R0, #1
                MOV     R1, #2
                BL      cacheLibInit
                BL      excVecInit
                BL      sysHwInit
                BL      sub_FF810B54
                BL      my_usrInit_1
                LDMDB   R11, {R11,SP,PC}


my_usrInit_1:											// original : sub_FF810B7C
                MOV     R12, SP
                STMFD   SP!, {R11,R12,LR,PC}
                SUB     R11, R12, #4
                SUB     SP, SP, #8
                BL      classLibInit
                BL      sub_FFD6B830
                LDR     R3, =0xF518
                LDR     R0, =0x240A0
                LDR     R1, [R3]
                LDR     R2, =0x248E0
                MOV     R3, #0x100
                BL      qInit
                LDR     R3, =0xF558
                LDR     R0, =0xF3BC						// activeQHead
                LDR     R1, [R3]
                BL      qInit
                LDR     R3, =0xF49C
                LDR     R0, =0x21460
                LDR     R1, [R3]
                BL      qInit
                BL      sub_FFD685BC
                BL      sub_FF810504
                MOV     R3, R0
                MOV     R2, #0xC00
                STR     R2, [SP,#0]
                MOV     R2, #0
                STR     R2, [SP,#4]
                LDR     R0, =my_usrRoot
                MOV     R1, #0x4000
                LDR     R2, =0x25530
                BL      sub_FFD6D6CC
                LDMDB   R11, {R11,SP,PC}

my_usrRoot:												// original : sub_FF8110BC
                MOV     R12, SP
                STMFD   SP!, {R11,R12,LR,PC}
                SUB     R11, R12, #4
                SUB     SP, SP, #8
                STR     R0, [R11,#-0x10]
                STR     R1, [R11,#0xFFFFFFEC]
                BL      sub_FF811004
                LDR     R0, [R11,#-0x10]
                LDR     R1, [R11,#0xFFFFFFEC]
                BL      sub_FFD76204
                LDR     R0, [R11,#-0x10]
                LDR     R1, [R11,#0xFFFFFFEC]
                BL      sub_FFD75A68
                BL      sub_FF810C50
                BL      sub_FF810B28
                MOV     R0, #0x32
                BL      selectInit
                BL      usrRoot_init0
                BL      usrRoot_init1
                BL      usrRoot_init2
                BL      selTaskDeleteHookAdd
                BL      sub_FFD890FC
                BL      my_usrAppInit
                LDMDB   R11, {R11,SP,PC}


my_usrAppInit:											// original : sub_FF810654
                MOV     R12, SP
                STMFD   SP!, {R11,R12,LR,PC}
                SUB     R11, R12, #4
                BL      my_task_Startup
                LDMDB   R11, {R11,SP,PC}


my_task_Startup:										// original : sub_FF811324
                STMFD   SP!, {R4,LR}
                MOV     R3, #0x5 // 386D4000
                SUB     SP, SP, #0x2C
                ADD     R3, R3, #0x380
                LDR     R2, =a2008_11_281217			// "2008.11.28 12:17:47"
                LDR     R1, =a4_0_1						// "4.0.1"
                LDR     R0, =aK190ReadyVerSB			// "--------------------------\nK190 READY :"
                STR     R3, [SP,#4]
                BL      printf
                BL      sub_FFD5BC68
                BL      sub_FFD3C2C0
                MOV     R1, #0xB80000 // reserve 512 KB or RAM for ML (original: 0xc00000)
                MOV     R0, #0x200000
                BL      sub_FFD5B350
                BL      InitializeKernel
                LDR     R0, [SP,#4]
                BL      sub_FFD59D94
                BL      sub_FFD444B0
                BL      FlushWriteCache_1
                BL      sub_FFD3E398
                MOV     R0, #0xF8000000
                BL      Install3VMemory
                BL      RegisterISRs_OCH
                MOV     R0, #0x32
                BL      InitializeTimerSystem
                LDR     R2, =0x1BC4
                MOV     R1, #8
                MOV     R0, #0x10
                BL      InitializeHPTimerSystem
                BL      ResourceNameServiceInit
                MOV     R0, #0
                BL      sub_FFD47BE4
                BL      sub_FFD409A0
                MOV     R0, #3
                BL      sysClockRateSet
                BL      InitializeSerialIO
                ADD     R4, SP, #8
                ADD     R0, SP, #4
                MOV     R1, R4
                BL      sub_FFD59FF4
                MOV     R0, R4
                BL      RtcInit
                BL      InitHPC
                LDR     R0, =sub_FF8199E0
                BL      PowerMgrInit
                BL      DisablePowerSave
                MOV     R2, #0x80000
                MOV     R1, #0x180000
                MOV     R0, #0x1F
                BL      sub_FFD4C08C
                MOV     R0, #0xFFFFFFFF
                MOV     R1, #3
                BL      sub_FFD4C590
                MOV     R1, #0
                LDR     R0, =aStartupeventfl			// "StartupEventFlag"
                BL      CreateEventFlag
				
                LDR     R3, =0xFCBC
                MOV     R4, #0
                MOV     R1, #0x18
                MOV     R2, #0x1800
                STR     R0, [R3]
				
                LDR     R3, =Startup_TaskMain
                LDR     R0, =aTaskmain					// "TaskMain"
                STR     R4, [SP,#0]
                BL      CreateTask

		//NOP
		//NOP
		//BL      CreateMyTasks
				
                BL      sub_FFD4C338					// register functions : dmstart, dmstop, dump, dumpf, ... dmprint, logstat
                BL      sub_FFD5CAE0					// register functions that control bootFlags - EnableBootDisk, DisableBootDisk,...
                BL      sub_FFD57564					// register functions : memShow, taskShow, checkSum,  

                MOV     R1, R4
                LDR     R0, =aUpdate					// "update"
                LDR     R2, =sub_FFD5C86C
                BL      Register_Func_with_Name
                MOV     R1, R4
                LDR     R0, =aLv_on					// "lv_on"
                LDR     R2, =sub_FF812728
                BL      Register_Func_with_Name
                MOV     R1, R4
                LDR     R0, =aSend_mpu_1				// "send_mpu"
                LDR     R2, =sub_FF811180
                BL      Register_Func_with_Name
				
                MOV     R1, R4
                LDR     R0, =aMarker_1					// "Marker"
                LDR     R2, =sub_FF8126EC
                BL      Register_Func_with_Name

		NOP
		NOP
		BL      CreateMyTasks
		NOP
		NOP

                LDR     R0, =aK190					// "K190"
                BL      sub_FFD4EC6C		
				
                ADD     SP, SP, #0x2C
                LDMFD   SP!, {R4,PC}


my_ledBlueOn:
                LDR   R0, =0xC02200E8
                LDR   R1, =0x46
                STR   R1, [R0]
                MOV   PC, LR

								
.align 2
fin:



