#ifndef _AudioIOunit_HEAD
#define _AudioIOunit_HEAD


#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <memory>
//#include "RingBufferA.h"

class AudioIOunit 
{
public:
    static AudioIOunit& instance();
    //void setParam();
    void init(float sampleRate, int channel, int sampleDeep);

    void startRec();
    void stopRec();
    void flushRecBuffers();
    bool isRecRunning();
//    bool popRecodeData(PopBufferChunkRef chunkref, size_t waitMicroSeconds = 0);
    
    
//   output
    //void setOutputFormat(float sampleRate, int channels, int sampleBitDeepth);
    void initOutput(float sampleRate, int channel, int sampleDeep);
    bool playbackIsRunning();
    void startPalyback();
    void stopPlayback();    
    void flushPlaybackBuffers();    
    bool isPlaybackRuning();
    unsigned char* getHungryPlayBuf(unsigned hungrySize);
    void putDataToPlayBuf(unsigned datasize);
    
    
    
    OSStatus startReadFile(const char* iosURL);
    OSStatus stopReadFile();
private:
    AudioIOunit();
    ~AudioIOunit();
private:
    std::auto_ptr<class AudioIOunit_context> _ctx;
};

#endif 