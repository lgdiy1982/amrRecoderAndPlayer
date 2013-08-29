//
//  SafePrinter.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/27/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#ifndef __AmrRecoderAndPlayer__SafePrinter__
#define __AmrRecoderAndPlayer__SafePrinter__

#include <IceUtil/IceUtil.h>
#include <list>
#include <BytesBuffer.h>
using namespace IceUtil;
using namespace std;
class SafePrinter : public IceUtil::Thread
{
public:
    static size_t feedCB(void* userData, const ChunkInfoRef,  bool terminated);
    static size_t eatCB(void* userData, const ChunkInfoRef,  bool terminated);
    SafePrinter();
    void printf(const char* format, ...);
    //void printf(const string& s);
    void run();
    void destroy();
    void waitforFinished();
private:
    list<string> _l;
    BytesBufferPtr _buffer;
    Monitor<Mutex> _monitor;
    bool _destroy;
    bool _waitforFinished;
    
};
typedef IceUtil::Handle<SafePrinter> SafePrinterPtr;

#endif /* defined(__AmrRecoderAndPlayer__SafePrinter__) */
