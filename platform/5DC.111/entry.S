/*##################################################################################
 #                                                                                 #
 #                          _____     _       _                                    #
 #                         |  ___|   | |     | |                                   #
 #                         |___ \  __| |_ __ | |_   _ ___                          #
 #                             \ \/ _` | '_ \| | | | / __|                         #
 #                         /\__/ / (_| | |_) | | |_| \__ \                         #
 #                         \____/ \__,_| .__/|_|\__,_|___/                         #
 #                                     | |                                         #
 #                                     |_|                                         #
 #                                                                                 #
 #################################################################################*/

/*##################################################################################
 #                                                                                 #
 #    Entry point for our code. This is the first thing that runs when the         #
 #    camera boots.                                                                #
 #                                                                                 #
 #################################################################################*/
    .text
    .org 0
    .globl _start, start, init_code_run

start:
_start:
        B  copy_and_restart



init_code_run:
TEQ     R0, #2

loc_800088:
        MOV     R1, #0x78
        MCR     p15, 0, R1,c1,c0, 0
        MOV     R1, #0
        MCR     p15, 0, R1,c7,c10, 4
        MCR     p15, 0, R1,c7,c7, 0
        MOV     R0, #0x40000006
        MCR     p15, 0, R0,c9,c1, 0
        MRC     p15, 0, R0,c1,c0, 0
        ORR     R0, R0, #0x10000
        MCR     p15, 0, R0,c1,c0, 0
        MRS     R1, CPSR
        BIC     R1, R1, #0x3F
        ORR     R1, R1, #0xD3
        MSR     CPSR_cf, R1
        LDR     R2, =0xC0200000
        MOV     R1, #0xFFFFFFFF
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

loc_800100:
        STR     R1, [R2,#0xDC]
        STR     R1, [R2,#0xEC]
        STR     R1, [R2,#0xFC]
        LDR     R1, =0xC0220000
        MOV     R2, #0x46
        STR     R2, [R1,#0x94]
        MOV     R1, #0xC0000000
        LDR     R2, =0xD9C50000
        STR     R2, [R1,#0x10]
        LDR     SP, =0x40000FFC
        MOV     R11, #0
        LDR     PC, =my_romStart
		
my_romStart:								
		MOV	R12, SP
		STMFD	SP!, {R4,R11,R12,LR,PC}
		SUB	R11, R12, #4
		MOV	R4, R0
		MOV	R3, #0x94               // MOV     R3, 0xC0220094 R3 = 0x94 + 0x220000 + 0xC0000000 = 0xC0220094		
		ADD	R3, R3, #0x220000
		ADD	R3, R3, #0xC0000000
		MOV	R2, #0x44
		STR	R2, [R3]
		LDR	R3, =0x2EF50
		SUB	R2, R3,	#0x1900
		MOV	R3, R2,LSR#2
		LDR	R0, =unk_FFB5CAD0
		MOV	R1, #0x1900
		MOV	R2, R3
		BL	sub_FF810318	
		MOV	R3, #0x94               // MOV     R3, 0xC0220094 R3 = 0x94 + 0x220000 + 0xC0000000 = 0xC0220094		
		ADD	R3, R3, #0x220000
		ADD	R3, R3, #0xC0000000
		MOV	R2, #0x46
		STR	R2, [R3]
		MOV	R0, R4
		BL	my_usrInit
		LDMDB	R11, {R4,R11,SP,PC}		
		
my_usrInit:
		MOV	R12, SP
		STMFD	SP!, {R11,R12,LR,PC}
		SUB	R11, R12, #4
		SUB	SP, SP,	#4
		STR	R0, [R11,#-0x10]
		LDR	R0, [R11,#-0x10]
		BL	sub_FF810E84             // sysStart
		MOV	R0, #1
		MOV	R1, #2
		BL	sub_FFB45FCC            // cacheLibInit
		BL	sub_FFB35004            // excVecInit
		BL	sub_FF810764            // sysHwInit
		BL	sub_FF810A64
		BL	my_usrInit_1
		LDMDB	R11, {R11,SP,PC}
		
my_usrInit_1:		
		MOV	R12, SP
		STMFD	SP!, {R11,R12,LR,PC}
		SUB	R11, R12, #4
		SUB	SP, SP,	#8
		BL	sub_FFB464CC     // classLibInit
		BL	sub_FFB56278     // taskLibInit
		LDR	R3, =0x11BAC     // qPriBMapClassId
		LDR	R0, =0x2DC0C     // readyQBMap
		LDR	R1, [R3]
		LDR	R2, =0x2E420     // readyQHead
		MOV	R3, #0x100
		BL	sub_FFB51D30     // qInit
		LDR	R3, =0x11B6C     // qFifoClassId
		LDR	R0, =0x11F0C     // activeQHead
		LDR	R1, [R3]
		BL	sub_FFB51D30     // qInit
		LDR	R3, =0x11C28     // qPriListClassId
		LDR	R0, =0x2B948     // tickQHead
		LDR	R1, [R3]
		BL	sub_FFB51D30     // qInit
		BL	sub_FFB5ADF8     // workQInit
		BL	sub_FF8108C0     // sysMemTop
		MOV	R3, R0
		MOV	R2, #0xC00
		STR	R2, [SP]
		MOV	R2, #0
		STR	R2, [SP,#4]
		LDR	R0, =my_usrRoot
		MOV	R1, #0x4000
		LDR	R2, =0x2EF50
		BL	sub_FFB534B8       //kernelInit
		LDMDB	R11, {R11,SP,PC}		
		
my_usrRoot:			
		MOV	R12, SP
		STMFD	SP!, {R11,R12,LR,PC}
		SUB	R11, R12, #4
		SUB	SP, SP,	#8
		STR	R0, [R11,#-0x10]
		STR	R1, [R11,#-0x14]
		BL	sub_FF810F14
		LDR	R0, [R11,#-0x10]
		LDR	R1, [R11,#-0x14]
		BL	sub_FFB4B664        // memInit
		LDR	R0, [R11,#-0x10]
		LDR	R1, [R11,#-0x14]
		BL	sub_FFB4C0DC        // memPartLibInit
		BL	sub_FF810B60
		BL	sub_FF810A38
		BL	sub_FF810F58
		BL	sub_FF810F3C
		BL	sub_FF810F84
		BL	sub_FFB33DFC       // cplusCtorsLink
		//BL  task_hooks         // <----this may not work
		BL	my_usrAppInit
		LDMDB	R11, {R11,SP,PC}	

my_usrAppInit:	
		MOV	R12, SP
		STMFD	SP!, {R11,R12,LR,PC}
		SUB	R11, R12, #4
		BL	my_task_Startup
		LDMDB	R11, {R11,SP,PC}	

my_task_Startup:			
		STMFD	SP!, {R4,R5,LR}
		MOV	R3, #0x38400000
		MOV	R5, #0x94
		ADD	R3, R3,	#0x2D4000
		ADD	R5, R5,	#0xC0000000
		SUB	SP, SP,	#0x2C
		ADD	R3, R3,	#0x380
		ADD	R5, R5,	#0x220000
		LDR	R0, =aRomReady
		STR	R3, [SP,#4]
		BL	sub_FFB48728          // printf
		BL	sub_FFB2B114
		BL	sub_FFB10D68
		MOV	R0, R5
		BL	sub_FFB291AC
		MOV	R1, #0x980000
		MOV	R0, #0x280000
		BL	sub_FFB2F5CC
		BL	sub_FFB18668        // InitializeKernel
		LDR	R0, [SP,#4]
		BL	sub_FFB2D814
		BL	sub_FFB189A4
		BL	sub_FFB1FA38       // FlushWriteCache
		BL	sub_FFB12828
		MOV	R0, #0xF8000000
		BL	sub_FFB2A3E0
		BL	sub_FFB14C44
		MOV	R0, #0x32
		BL	sub_FFB1FDAC      // InitializeTimerSystem
		LDR	R2, =0x1B40
		MOV	R1, #8
		MOV	R0, #0x10
		BL	sub_FFB136CC     // InitializeHPTimerSystem
		BL	sub_FFB1DC44     // ResourceNameServiceInit
		MOV	R0, #0
		BL	sub_FFB1C004
		BL	sub_FFB14F6C
		MOV	R0, #3
		BL	sub_FFB29918     // sysClockRateSet_100
		BL	sub_FFB2979C     // InitializeSerialIO
		ADD	R4, SP,	#8
		ADD	R0, SP,	#4
		MOV	R1, R4
		BL	sub_FFB2DA7C
		MOV	R0, R4
		BL	sub_FFB110BC
		BL	sub_FFB2CB40
		MOV	R0, #1
		BL	sub_FFB2DBFC
		BL	sub_FFB2DC58
		MOV	R0, R5
		BL	sub_FFB291E8
		MOV	R2, #0x180000
		MOV	R3, #0x80000
		MOV	R1, #0x1F
		MOV	R0, #2
		BL	sub_FFB20534     // DebugMenager
		MOV	R1, #7
		MOV	R0, #0xFFFFFFFF
		BL	sub_FFB20A5C
		MOV	R0, R5
		BL	sub_FFB291AC
		LDR	R0, =sub_FF812018
		MOV	R1, #0
		BL	sub_FFB2DAF4
		LDR	R3, =off_FF810FF4
		LDR	R4, [R3]
		MOV	R1, #0xA
		MOV	R0, R4
		BL	sub_FFB16D84    // CreateMessageQueue
		LDR	R3, =0x1B70
		ANDS	R5, R0,	#1
		STR	R0, [R3]
		BEQ	loc_FF8111C0
		MOV	R3, R0
		MOV	R1, #6
		MOV	R0, #0
		LDR	R2, =aStartupErrorCr
		BL	sub_FFB20698
		B	loc_FF81121C		

loc_FF8111C0:				
		LDR	R3, =sub_FF81138C
		MOV	R0, R4
		MOV	R1, #0x17
		MOV	R2, #0x1800
		STR	R5, [SP]
		BL	sub_FFB18B40    // CreateTask
                            // BL  DisablePowerSave
        BL  Create5dplusInit
		MOV	R3, R0
		ANDS	R0, R3,	#1
		BEQ	loc_FF8111F8
		MOV	R0, R5
		MOV	R1, #6
		LDR	R2, =aStartupError_0
		BL	sub_FFB20698
		B	loc_FF81121C

loc_FF8111F8:			
		MOV	R1, #6
		LDR	R2, =aIcuFirmwareVer
		LDR	R3, =0x1B68
		BL	sub_FFB20698
		BL	sub_FF8B7DB0
		BL	sub_FFB207B8
		BL	sub_FFB2AD78
		LDR	R0, =aK213
		BL	sub_FFB22394

loc_FF81121C:	
		ADD	SP, SP,	#0x2C
		LDMFD	SP!, {R4,R5,PC}		
		
		
.align 2	



