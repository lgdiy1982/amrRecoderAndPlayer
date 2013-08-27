//
//  HexDump.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/27/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#ifndef AmrRecoderAndPlayer_HexDump_h
#define AmrRecoderAndPlayer_HexDump_h
#include <stdio.h>

template <class T>
void displayHexBin(const T& v, bool hex = true)
{
    const unsigned char c2h[] = "0123456789ABCDEF";
    const unsigned char c2b[] = "01";
    
    unsigned char* p = (unsigned char*)&v;
    char* buf = new char [sizeof(T)*2+1];
    char* ptmp = buf;
    if(hex)
    {
        p = p + sizeof(T)-1;
        for (size_t i = 0; i < sizeof(T); i++, --p)
        {
            *buf++ = c2h[*p >> 4];
            *buf++ = c2h[*p & 0x0F];
        }
        *buf = '\0';
        //printf("hex format displayed as %s\n", ptmp);
        printf("%s", ptmp);
        
        delete [] ptmp;
        
    }
    else
    {
        p = (unsigned char*)&v;
        p = p + sizeof(T)-1;
        ptmp = buf = new char [sizeof(T)*8+1];
        for (size_t i = 0; i < sizeof(T); i++, --p)
        {
            for (int j = 0; j < 8; j++)
                *buf++ = c2b[(*p >> (7-j)) & 0x1];
        }
        *buf = '\0';
        //printf("bin format displayed as %s\n", ptmp);
        printf("%s", ptmp);
        delete [] ptmp;
    }
    
}


void bytes2HexS(const unsigned char* src, size_t size);

#endif
