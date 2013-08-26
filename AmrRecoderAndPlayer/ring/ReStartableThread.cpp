#include "ReStartableThread.h"
#include <IceUtil/Thread.h>
#include <IceUtil/Monitor.h>
#include <IceUtil/Mutex.h>
#include <stdio.h>
#include <string>
using namespace std;
using namespace IceUtil;

#define DBGRestartableThread
enum ThreadState
{
    e_null,
    e_starting/* = 'sttg'*/,
    e_started/* =  'sttd'*/,
    e_stopping/* = 'stpg'*/,
    e_stopped/* =  'stpd'*/,
    e_destroying/* = 'dstg'*/,
    e_destroyed/* = 'dstd'*/
};


class RestartableThread_context
{
    friend class RestartableThread;
public:
    RestartableThread_context(RestartableThread * const ptr);
    ~RestartableThread_context();
    void StartAccomplished();
    //virtual void singleLoopTask() {}
    void restart(bool syn = true);
    void stop(bool syn = true);
    void destroy(bool syn = true);
    ThreadState state();
    virtual void run();
private:
    volatile bool _threadDestroy;
    volatile bool _loopStarted;

    Monitor<Mutex> _stateMonitor;
    Monitor<Mutex> _monitor;
    ThreadState _state;
    ThreadState        _startState;
    ThreadState        _stopState;
    ThreadState        _destroyState;
    RestartableThread* _that;
    string             _threadName;
};



RestartableThread_context::RestartableThread_context(RestartableThread * const ptr)
{
    _that = ptr;
    _threadDestroy = false;
    _loopStarted = false;
    _state = e_stopped;
    
    _startState = e_null;
    _stopState = e_stopped;
    _destroyState = e_null;    

}

RestartableThread_context::~RestartableThread_context()
{

}

ThreadState RestartableThread_context::state()
{
    Monitor<Mutex>::Lock lock(_stateMonitor);
    return _state;
}


#if 0
void RestartableThread_context::restart(bool syn)
{

    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        if(_state != e_stopped)
            return;
        _state = e_starting;
#ifdef DBGRestartableThread        
        printf("::restarting\n");
#endif
    }

    {
        Monitor<Mutex>::Lock lock(_monitor);
        _loopStarted = true;
        _monitor.notify();
    }

    if(syn)
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        while(_state != e_started)
            _stateMonitor.wait();
    }
#ifdef DBGRestartableThread        
    printf("::restarted\n");
#endif    
}

void RestartableThread_context::stop(bool syn)
{
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        if(_state != e_started)
            return ;
#ifdef DBGRestartableThread        
        printf("::stopping\n");
#endif        
        _state = e_stopping;
        _that->stopCondition();
    }


    if(syn)
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        while(_state != e_stopped)
            _stateMonitor.wait();
    }
#ifdef DBGRestartableThread     
    printf("::stopped\n");
#endif    
}

void RestartableThread_context::destroy(bool syn)
{

    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        if(_state != e_stopped)
            return;
#ifdef DBGRestartableThread        
        printf("::destroying\n");
#endif         
        _state = e_destroying;
    }

    {
        Monitor<Mutex>::Lock lock(_monitor);
        _threadDestroy = true;
        _monitor.notify();
    }


    if(syn)
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        while(_state != e_destroyed)
            _stateMonitor.wait();
    }
#ifdef DBGRestartableThread    
    printf("::destroyed\n");
#endif     
}

void RestartableThread_context::StartAccomplished()
{
    if(_state == e_starting && _loopStarted)
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        _state = e_started;
        _stateMonitor.notify();
    }
}

void RestartableThread_context::run()
{
    ThreadState monitorState;

    while(1)
    {

        {
            Monitor<Mutex>::Lock lock(_monitor);
            while(!_loopStarted && !_threadDestroy)
            {
#ifdef DBGRestartableThread                
                printf("%s waitting!! startedAct:%d destroyAct:%d\n", _threadName.c_str(), _loopStarted, _threadDestroy );
#endif                 
                _monitor.wait();
#ifdef DBGRestartableThread                
                printf("%s notified!! startedAct:%d destroyAct:%d\n", _threadName.c_str(), _loopStarted, _threadDestroy );
#endif                 
            }
        }
#ifdef DBGRestartableThread        
            printf("out waitting!!!\n");
#endif         

            {
                Monitor<Mutex>::Lock lock(_stateMonitor);
                monitorState = _state;
            }

            if(_state == e_destroying && _threadDestroy)
                break;

//            if(_state == e_starting && _loopStarted)
//            {
//                Monitor<Mutex>::Lock lock(_stateMonitor);
//                _state = e_started;
//                _stateMonitor.notify();
//            }

        _that->singleLoopTask();
#ifdef DBGRestartableThread        
        printf("%s task ended!!!!\n", _threadName.c_str());
#endif         

        {
            Monitor<Mutex>::Lock lock(_monitor);
            _loopStarted = false;
        }

        {
            Monitor<Mutex>::Lock lock(_stateMonitor);
            if(_state == e_stopping)
            {
                _state = e_stopped;
                _stateMonitor.notify();
#ifdef DBGRestartableThread                
                printf("%s notify stop!!!\n", _threadName.c_str());
#endif                 
            }
            else
            {
                _state = e_stopped;
#ifdef DBGRestartableThread                
                printf("%s normal stop!!!\n", _threadName.c_str());
#endif                 
            }

        }

    }


    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        _state = e_destroyed;
        _stateMonitor.notify();
    }
    //_that->getThreadControl().join();
    //printf("destroyed .... \n");
}
#else   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RestartableThread_context::restart(bool syn)
{
    
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        if(_stopState != e_stopped)
            return;
        _startState = e_starting;
#ifdef DBGRestartableThread        
        printf("%s ::restarting\n", _threadName.c_str());
#endif
    }
    
    {
        Monitor<Mutex>::Lock lock(_monitor);
        _loopStarted = true;
        _monitor.notify();
    }
    
    if(syn)
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        while(_startState != e_started)
            _stateMonitor.wait();
    }
#ifdef DBGRestartableThread        
    printf("%s ::restarted\n", _threadName.c_str());
#endif    
}

void RestartableThread_context::stop(bool syn)
{
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        if(_startState != e_started)
            return ;
#ifdef DBGRestartableThread        
        printf("%s ::stopping\n", _threadName.c_str());
#endif        
        _startState = e_null;
        _stopState = e_stopping;
        _state = e_stopping;
        _that->stopCondition();
    }
    
    
    if(syn)
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        while(_stopState != e_stopped)
            _stateMonitor.wait();
    }
#ifdef DBGRestartableThread     
    printf("%s ::stopped\n", _threadName.c_str());
#endif    
}

void RestartableThread_context::destroy(bool syn)
{
    
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        if(_stopState != e_stopped)
            return;
#ifdef DBGRestartableThread        
        printf("%s ::destroying\n", _threadName.c_str());
#endif   
        _stopState = e_null;
        _destroyState = e_destroying;
        _state = e_destroying;
    }
    
    {
        Monitor<Mutex>::Lock lock(_monitor);
        _threadDestroy = true;
        _monitor.notify();
    }
    
    
    if(syn)
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        while(_destroyState != e_destroyed)
            _stateMonitor.wait();
    }
#ifdef DBGRestartableThread    
    printf("%s ::destroyed\n", _threadName.c_str());
#endif     
}

void RestartableThread_context::StartAccomplished()
{
    if(_startState == e_starting && _loopStarted)
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        _startState = e_started;
        _state = e_started;
        _stateMonitor.notify();
    }
}

void RestartableThread_context::run()
{  
    while(1)
    {
        
        {
            Monitor<Mutex>::Lock lock(_monitor);
            while(!_loopStarted && !_threadDestroy)
            {
#ifdef DBGRestartableThread                
                printf("%s waitting!! startedAct:%d destroyAct:%d\n",_threadName.c_str(),  _loopStarted, _threadDestroy );
#endif                 
                _monitor.wait();
#ifdef DBGRestartableThread                
                printf("%s notified!! startedAct:%d destroyAct:%d\n",_threadName.c_str(),  _loopStarted, _threadDestroy );
#endif                 
            }
        }
#ifdef DBGRestartableThread        
        printf("%s out waitting!!!\n", _threadName.c_str());
#endif         

        
        if(_destroyState == e_destroying && _threadDestroy)
            break;
        
        _that->singleLoopTask();
#ifdef DBGRestartableThread        
        printf("%s task ended!!!!\n", _threadName.c_str());
#endif         
        
        {
            Monitor<Mutex>::Lock lock(_monitor);
            _loopStarted = false;       //could be flush before go to the while 
        }
        
        {
            Monitor<Mutex>::Lock lock(_stateMonitor);
            if(_stopState == e_stopping)
            {
                _stopState = e_stopped;
                _stateMonitor.notify();
#ifdef DBGRestartableThread                
                printf("%s notify stop!!!\n", _threadName.c_str());
#endif                 
            }
            else
            {
                _startState = e_null;
                _stopState = e_stopped;
#ifdef DBGRestartableThread                
                printf("%s normal stop!!!\n", _threadName.c_str());
#endif                 
            }
            
        }
        
    }
    
    
    {
        Monitor<Mutex>::Lock lock(_stateMonitor);
        _destroyState = e_destroyed;
        _stateMonitor.notify();
    }
}


#endif



//////////////////////////////////////////////////////////////////////////////////////////////////////////////



RestartableThread::RestartableThread(const char* threadName)
{
    _ctx = auto_ptr<RestartableThread_context> (new RestartableThread_context(this) );
    _ctx->_that->start();
    if(threadName)
        _ctx->_threadName = threadName;
}

RestartableThread::~RestartableThread()
{

}


void RestartableThread::restart()
{
    _ctx->restart(true);
}

void RestartableThread::StartAccomplished()
{
    _ctx->StartAccomplished();
}

void RestartableThread::stop()
{
    _ctx->stop(true);
}

bool RestartableThread::isRunning()
{
    ThreadState state = _ctx->state();
    if(state == e_started || state == e_starting)
        return true;
    else
        return false;
}

void RestartableThread::destroy()
{
    _ctx->destroy(true);
}

void RestartableThread::run()
{
    _ctx->run();
}

