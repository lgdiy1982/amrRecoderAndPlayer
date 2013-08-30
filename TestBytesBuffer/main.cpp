//
//  main.cpp
//  TestBytesBuffer
//
//  Created by lu gang on 8/27/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#include <iostream>
#include <IceUtil/IceUtil.h>
#include <list>
#include <string>
#include <BytesBuffer.h>
#include <SP.h>
using namespace IceUtil;
using namespace std;



//--------------------------------------------------------------------------------------------
SPPtr g_printer;

BytesBufferPtr bufferPtr;

//--------------------------------------------------------------------------------------------
class EatThread : public IceUtil::Thread    //eat data, write to a file
{
public:
    static size_t cb(void* userData, const ChunkInfoRef info, bool feedTerminated)
    {
        EatThread* thiz = (EatThread*)userData;
        if (feedTerminated && info->_data == 0)
        {
            thiz->_destroy = true;
            g_printer->printf("<<<<<--------XXXXXXXXXXX\n");
            return 0;
        }
        size_t ret = ::fwrite(info->_data, sizeof(unsigned char), info->_size, thiz->fo);
        
        
        static size_t count = 0;
        count += info->_size;
        //g_printer->printf("<<<<<--------:%d\n", count);
                

        return ret;
    }
    
    EatThread(const string& path):_path(path)
    {
        chunk._userData = this;
        chunk._callback = EatThread::cb;
        _destroy = false;
    }
    
    virtual void run()
    {
        fo = ::fopen(_path.c_str(), "wb+");
        do {
            bufferPtr->eat(100, &chunk);
        }while (!_destroy);
        fclose(fo);
    }
private:
    BufferChunk chunk;
    FILE *fo;
    bool _destroy;
    string _path;
};

typedef Handle<EatThread> EatThreadPtr;
EatThreadPtr eatter;
//--------------------------------------------------------------------------------------------
class FeedThread: public IceUtil::Thread {  //read a file
public:
    static size_t cb(void* userData, const ChunkInfoRef info,  bool EatTerminated)
    {
        FeedThread *thiz = (FeedThread*)userData;
        size_t ret = fread(info->_data, sizeof(unsigned char), info->_size, thiz->fi);
        static size_t count = 0;
        count += ret;
        //g_printer->printf("--------------->>>>>:%d\n", count);
        if (ret < info->_size ) {
            bufferPtr->terminatedFeed();
            g_printer->printf("------------>>>>>XXXXXXXXX\n");
            thiz->_destroy = true;
        }
        if (EatTerminated) {
            thiz->_destroy = true;
            g_printer->printf(">>>>>XXXXXXXXX\n");
        }
        return  ret;
    }
    
    FeedThread(const string& path):_path(path)
    {
        chunk._userData = this;
        chunk._callback = FeedThread::cb;
        _destroy = false;
    }
    virtual void run()
    {
        fi = ::fopen(_path.c_str(), "rb+");
        do {
            bufferPtr->feed(100, &chunk);
        }while (!_destroy);
        fclose(fi);
    }
public:
    BufferChunk chunk;
    FILE *fi;
    bool _destroy;
    string _path;
};
typedef Handle<FeedThread> FeedThreadPtr;

int main(int argc, const char * argv[])
{
    g_printer = new SP();
    g_printer->start();
    bufferPtr = new BytesBuffer((2<<10)-48);
    //read
    FeedThreadPtr feeder = new FeedThread("aaaa.png");
    EatThreadPtr eatter = new EatThread("copy.png");
    feeder->start();
    eatter->start();
    feeder->getThreadControl().join();
    eatter->getThreadControl().join();
    g_printer->printf("done\n");
    //terminalptr->destroy();
    g_printer->getThreadControl().join();
    return 0;
}

