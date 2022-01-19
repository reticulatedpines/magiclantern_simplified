private sub Initialize()
    ExportToEventProcedure("dumpMemoryToFile", 0xE02A3E62|1)
    dumpMemoryToFile("DFFC0000_", 0xDFFC0000, 0x40000, 1)
end sub