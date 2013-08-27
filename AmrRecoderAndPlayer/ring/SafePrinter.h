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
using namespace IceUtil;
using namespace std;
class SafePrinter : public IceUtil::Thread
{
public:
    SafePrinter();
    void printf(const char* format, ...);
    void run();
    void destroy();
    void waitforFinished();
private:
    list<string> _l;
    Monitor<Mutex> _monitor;
    bool _destroy;
    bool _waitforFinished;
};
typedef IceUtil::Handle<SafePrinter> SafePrinterPtr;

#endif /* defined(__AmrRecoderAndPlayer__SafePrinter__) */
