#ifndef _AudioPlayUnit_HEAD
#define _AudioPlayUnit_HEAD

#include <memory>


struct PlaybackListener{
    void* userData;
    void (*progress)(void* userData, double expired);
    void (*finish)(void* userData);
};

#ifdef __cplusplus
extern "C"
#endif
int parseAmrFileDuration(const char* filepath);

class AudioPlayUnit
{
public:
    static AudioPlayUnit& instance();
    bool startPlay(const char* filepath);
    bool stopPlay();
    bool pausePlay();
    bool resume();
    bool isRunning();
    void setPlaybackListener(const PlaybackListener& listener);
private:
    AudioPlayUnit();
    ~AudioPlayUnit();
private:
    std::auto_ptr<class AudioPlayUnit_context> _ctx;
};

#endif