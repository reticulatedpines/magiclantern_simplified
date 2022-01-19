' https://www.magiclantern.fm/forum/index.php?topic=25305.msg230668#msg230668
' firmware function addresses 
' R6.150 addresses
dim a_fio_readf=0xE07B32D4 ' FIO_ReadFile
dim a_fio_writef=0xE07B37BC ' FIO_WriteFile
dim a_fio_removf=0xE07B3240 ' FIO_RemoveFile (newer cams have an equivalent RemoveFile eventproc)
dim a_umalloc=0xE07C16C0 ' AllocateUncacheableMemory, called _alloc_dma_memory in stubs.s
dim a_ufree=0xE07C16C6 ' used to free UncacheableMemory allocations (not named) _free_dma_memory in stubs.s

dim cpuinf_binary="B:/CPUINFO.BIN"
dim cpuinf_data="B:/CPUINFO.DAT"

private sub reg_funcs1()
    ExportToEventProcedure("FIO_ReadFile",a_fio_readf|1)
    ExportToEventProcedure("FIO_WriteFile",a_fio_writef|1)
    ExportToEventProcedure("FIO_RemoveFile",a_fio_removf|1)
    ExportToEventProcedure("umalloc",a_umalloc|1)
    ExportToEventProcedure("ufree",a_ufree|1)
end sub

private sub Initialize()
    reg_funcs1()
    buf = umalloc(0x1000)
    if buf=0 then
        exit sub
    end if
    bufo = umalloc(0x1000)
    if bufo=0 then
        ufree(buf)
        exit sub
    end if
    memset(bufo,0,0x1000)
    f = OpenFileRD(cpuinf_binary)
    FIO_ReadFile(f,buf,0x1000)
    CloseFile(f)
    ExportToEventProcedure("cpuinfo",buf|1)
    cpuinfo(bufo)
    FIO_RemoveFile(cpuinf_data)
    f = OpenFileCREAT(cpuinf_data)
    CloseFile(f)
    f = OpenFileWR(cpuinf_data)
    FIO_WriteFile(f,bufo,0x1000)
    CloseFile(f)
    ufree(bufo)
    ufree(buf)
end sub