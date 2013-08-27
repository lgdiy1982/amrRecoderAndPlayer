//
//  SafePrinter.cpp
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/27/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#include "SafePrinter.h"


SafePrinter::SafePrinter() : _destroy(false), _waitforFinished(false) {}


void SafePrinter::printf(const char* format, ...)
{
    char buf[2<<8];
    bzero(buf, sizeof(buf));
    Monitor<Mutex>::Lock lock(_monitor);
    va_list vl;
    va_start(vl, format);
    vsnprintf(buf, 128, format, vl);
    va_end(vl);
    
    if (_l.empty()) {
        _monitor.notify();
    }
    _l.push_front(string(buf) );
}



void SafePrinter::run()
{
    while (true) {
        {
            Monitor<Mutex>::Lock lock(_monitor);
            while ((_l.empty() && !_destroy) || (!_l.empty()&&_waitforFinished) ) {
                _monitor.wait();
            }
        }
        
        string  msg;//
        {
            Monitor<Mutex>::Lock lock(_monitor);
            msg = _l.back();
            _l.pop_back();
        }
        cout << msg ;
    }
}


void SafePrinter::destroy()
{
    Monitor<Mutex>::Lock lock(_monitor);
    _destroy = true;
    _monitor.notify();
}

void SafePrinter::waitforFinished()
{
    Monitor<Mutex>::Lock lock(_monitor);
    _waitforFinished = true;
    _monitor.notify();
}



