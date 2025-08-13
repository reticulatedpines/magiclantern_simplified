
#include <idc.idc>
static main()
{
    auto ea, str, count, ref, offset;
    auto end;
    auto teststr;
    auto entries = 0;
    auto named = 0;

    /* starting address for export, this one should work with any start address we use */
    ea = 0xF8000000;
    /* apply an offset in case the segment was set to the wrong address */
    //offset = 0x07800000;
    offset = 0;

    auto file = fopen("export.csv", "w");
    while( ea != BADADDR )
    {
        str = GetFunctionName(ea);
        if( str != 0 )
        {
            end = FindFuncEnd(ea);

            count = 0;
            ref = RfirstB(ea);
            while(ref != BADADDR)
            {
                count = count + 1;
                ref = RnextB(ea, ref);
            }

            teststr = sprintf("sub_%X", ea);
            if( teststr != str )
            {
                named++;
            }
            entries++;
            fprintf(file, "0x%X;%s;%s;\n", ea + offset, str, GetType(ea) );
        }
        ea = NextFunction(ea);
    }
    fclose(file);
    Message("Done, exported %d entries, %d with manual names\n", entries, named);
}
