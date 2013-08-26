#ifndef RestartableThread_HEAD
#define RestartableThread_HEAD
#include <memory>
#include <IceUtil/Thread.h>
class RestartableThread : public IceUtil::Thread
{
public:
    RestartableThread(const char* threadName = 0);
    ~RestartableThread();
    virtual void singleLoopTask() = 0;
    virtual void stopCondition() = 0;
    void restart();
    
    void stop();
    bool isRunning();
    void destroy();
protected:
    void StartAccomplished();
private:
    void run();
private:
    std::auto_ptr<class RestartableThread_context> _ctx;
};




#endif
