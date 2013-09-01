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

    void start(const char* path);
    void stop();
    void cancel();
    
    void flush();
    bool isRunning();

private:
    AudioInputUnit();
    ~AudioInputUnit();
private:
    std::auto_ptr<class AudioInputUnit_context> _ctx;
};


#endif 