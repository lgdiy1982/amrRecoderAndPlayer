//
//  SP.cpp
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/27/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#include "SP.h"
#include <IceUtil/Handle.h>
#include <IceUtil/Thread.h>
#include <IceUtil/Monitor.h>
#include <list>


using namespace IceUtil;
using namespace std;


typedef Handle<class SP_context>  SP_context_Ptr;
class SP_context: public IceUtil::Thread
{
public:
    static SP_context_Ptr instance();
    SP_context();
    ~SP_context();
    void printf(const char* format, ...);
    void printf(const char* format, va_list vl);
    void destroy();
    void waitforFinished();
    void run();
private:
    list<string> _l;
    static Monitor<Mutex> _monitor;
    bool _destroy;
    bool _waitforFinished;
};

Monitor<Mutex> SP_context::_monitor;

SP_context_Ptr SP_context::instance()
{
    static SP_context_Ptr instance;
    Monitor<Mutex>::Lock lock(_monitor);
    if (!instance.get()) {
        instance = new SP_context();
        instance->start();
    }
    return instance;
}

SP_context::SP_context()
:_destroy(false)
,_waitforFinished(false)
{
    
}


SP_context::~SP_context()
{
    Monitor<Mutex>::Lock lock(_monitor);
    _waitforFinished = true;
}

void SP_context::printf(const char* format, ...)
{
    char buf[2<<8];
    bzero(buf, sizeof(buf));
    va_list vl;
    va_start(vl, format);
    vsnprintf(buf, sizeof(buf), format, vl);
    va_end(vl);
    Monitor<Mutex>::Lock lock(_monitor);
    if (_l.empty()) {
        _monitor.notify();
    }
    _l.push_front(string(buf));
}

void SP_context::printf(const char* format, va_list vl)
{
    char buf[2<<8];
    bzero(buf, sizeof(buf));
    vsnprintf(buf, sizeof(buf), format, vl);
    Monitor<Mutex>::Lock lock(_monitor);
    if (_l.empty()) {
        _monitor.notify();
    }
    _l.push_front(string(buf));
}

void SP_context::run()
{
    while (true) {
        {
            Monitor<Mutex>::Lock lock(_monitor);
            while (_l.empty() && !_destroy && !_waitforFinished) {
                _monitor.wait();
            }
        }
        
        if(_destroy)
            break;
        
        string msg;//
        {
            Monitor<Mutex>::Lock lock(_monitor);
            msg = _l.back();
            _l.pop_back();
        }
        cout << msg ;
        {
            Monitor<Mutex>::Lock lock(_monitor);
            if (_l.empty() && _waitforFinished)
                break;            
        }
    }
}

void SP_context::destroy()
{
    Monitor<Mutex>::Lock lock(_monitor);
    _destroy = true;
    _monitor.notify();
}

void SP_context::waitforFinished()
{
    Monitor<Mutex>::Lock lock(_monitor);
    _waitforFinished = true;
    _monitor.notify();
}





//------------------------------------------------------------------------------------------------------------------------------------------------------------
SP::SP()
{
    
}

void SP::printf(const char* format, ...)
{
    char buf[2<<8];
    bzero(buf, sizeof(buf));
    va_list vl;
    va_start(vl, format);
    SP_context::instance()->printf(format, vl);
    va_end(vl);
}


