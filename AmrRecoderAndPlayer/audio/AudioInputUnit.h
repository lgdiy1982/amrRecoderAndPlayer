#ifndef _AudioInputUnit_HEAD
#define _AudioInputUnit_HEAD

#include <memory>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

#ifdef __cplusplus
extern "C"
{
#endif
    
    struct RecordListener{
        void* userData;
        void (*progress)(void* userData, double totalDuration);
        void (*finish)(void* userData, double duration);
        void (*updateMeter) (void* userData, float averageMeter, size_t channel);
    };
#ifdef __cplusplus
} //end extern "C"
#endif




class AudioInputUnit
{
public:
    static AudioInputUnit& instance();
    bool start(const char* path);
    bool stop();
    bool cancel();
    
    //bool isRunning();
    void setRecordListener(const RecordListener& listener);
private:
    AudioInputUnit();
    ~AudioInputUnit();
private:
    std::auto_ptr<class AudioInputUnit_context> _ctx;
};


#endif