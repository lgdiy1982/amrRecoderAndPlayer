#ifndef _AudioInputUnit_HEAD
#define _AudioInputUnit_HEAD
#include "ReStartableThread.h"
#include <memory>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>


class AudioInputUnit 
{
public:
    static AudioInputUnit& instance();
    bool start(const char* path);
    bool stop();
    bool cancel();

    bool isRunning();

private:
    AudioInputUnit();
    ~AudioInputUnit();
private:
    std::auto_ptr<class AudioInputUnit_context> _ctx;
};


#endif 