    
#include <idc.idc>
static main()
{
    auto ea, str, count, ref;
    auto end;
    auto teststr;
    auto entries = 0;

    ea = 0xFF000000;

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
                entries++;
                fprintf(file, "0x%X;%s;%s\n", ea, str, GetType(ea) );
            }
        }
        ea = NextFunction(ea);
    }
    fclose(file);
    Message("Done, exported %d entries\n", entries);
}
