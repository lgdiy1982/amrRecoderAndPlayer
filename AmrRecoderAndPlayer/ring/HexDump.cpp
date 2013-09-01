#include "HexDump.h"


void bytes2HexS(const unsigned char* src, size_t size)
{
    static size_t totalcount = 0;
    for (unsigned i=0; i <size; ++i)
    {
        if (totalcount++%16 == 0)
            SP::printf("\n%0lx0h:", totalcount/16);
        displayHexBin( src[i]);
        SP::printf(" ");
    }
    totalcount = 0;
}