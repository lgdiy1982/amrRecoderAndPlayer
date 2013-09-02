#ifndef _AudioInputUnit_HEAD
#define _AudioInputUnit_HEAD
#include "ReStartableThread.h"
#include <memory>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

struct RecordListener{
    void* userData;
    void (*progress)(void* userData, double totalDuration);
    void (*finish)(void* userData);
};

class AudioInputUnit 
{
public:
    static AudioInputUnit& instance();
    bool start(const char* path);
    bool stop();
    bool cancel();

    bool isRunning();
    void setRecordListener(const RecordListener& listener);
private:
    AudioInputUnit();
    ~AudioInputUnit();
private:
    std::auto_ptr<class AudioInputUnit_context> _ctx;
};


#endif 