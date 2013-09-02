#ifndef _AudioPlayUnit_HEAD
#define _AudioPlayUnit_HEAD

#include <memory>


struct PlaybackListener{
    void* userData;
    void (*progress)(void* userData, double currentTimeStamp, double totalDuration);
    void (*finish)(void* userData);
};

class AudioPlayUnit
{
public:
    static AudioPlayUnit& instance();
    bool startPlay(const char* filepath);
    bool stopPlay();
    bool isRunning();
    void setPlaybackListener(const PlaybackListener& listener);
private:
    AudioPlayUnit();
    ~AudioPlayUnit();
private:
    std::auto_ptr<class AudioPlayUnit_context> _ctx;
};

#endif 