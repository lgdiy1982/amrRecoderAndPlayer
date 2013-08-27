#ifndef _AudioPlayUnit_HEAD
#define _AudioPlayUnit_HEAD
//#include "ReStartableThread.h"
#include <memory>
//#include "RingBufferA.h"
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>


class AudioPlayUnit
{
public:
    static AudioPlayUnit& instance();
    void initialize(float sampleRate, int channel, int sampleDeep);
    void uninitialize();
    bool isInitialized();
    OSStatus startPlay();
    OSStatus stopPlay();
    void flush();
    bool isRunning();
    unsigned char* getBuffer(unsigned & limitSize, unsigned waitTimeMicroSeconds = 0);
    void fillBuffer(unsigned bufferSize);
private:
    AudioPlayUnit();
    ~AudioPlayUnit();
private:
    std::auto_ptr<class AudioPlayUnit_context> _ctx;
};

#endif 