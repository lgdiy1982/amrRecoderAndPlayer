//
//  SafePrinter.cpp
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/27/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#include "SafePrinter.h"
SafePrinter::SafePrinter() :
_destroy(false), _waitforFinished(false)
//, _buffer(new BytesBuffer(2<<10))
{}

//size_t SafePrinter::feedCB(void* userData, const ChunkInfoRef,  bool terminated)
//{
//    
//}
//
//size_t SafePrinter::eatCB(void* userData, const ChunkInfoRef,  bool terminated)
//{
//    
//}

//void SafePrinter::run()
//{
//    do{
//        //_buffer->eat(<#size_t size#>, <#BufferChunkRef cbChunk#>)
//        if (_waitforFinished && _buffer->empty()) {
//            break;
//        }
//    }while(_destroy);
//}



void SafePrinter::printf(const char* format, ...)
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



void SafePrinter::run()
{
    while (true) {
        {
            Monitor<Mutex>::Lock lock(_monitor);
            while (_l.empty() && !_destroy) {
                _monitor.wait();
            }
        }
        
        if(_destroy)
            break;
        
        
        string  msg;//
        {
            Monitor<Mutex>::Lock lock(_monitor);
            msg = _l.back();
            _l.pop_back();
        }
        cout << msg ;
        if (_waitforFinished) {
            break;
        }
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
}



