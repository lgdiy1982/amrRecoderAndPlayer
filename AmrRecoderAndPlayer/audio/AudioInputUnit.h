#ifndef _AudioInputUnit_HEAD
#define _AudioInputUnit_HEAD
#include "ReStartableThread.h"
#include <memory>
//#include "RingBufferA.h"
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>


class AudioInputUnit 
{
public:
    static AudioInputUnit& instance();
    void initialize(float sampleRate, int channel, int sampleDeep);
    void uninitialize();
    bool isInitialized();

    OSStatus startRec();
    OSStatus stopRec();
    void flush();
    bool isRunning();
    //bool getData(PopBufferChunkRef chunkref, size_t waitMicroSeconds = 0);
    
    
    OSStatus startReadFile(const char* iosURL);
    OSStatus stopReadFile();
private:
    AudioInputUnit();
    ~AudioInputUnit();
private:
    std::auto_ptr<class AudioInputUnit_context> _ctx;
};

#endif 