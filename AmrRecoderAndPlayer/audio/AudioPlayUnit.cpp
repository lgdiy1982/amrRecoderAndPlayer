#include "AudioPlayUnit.h"

#import <AudioToolbox/AudioToolbox.h>
#include <IceUtil/Time.h>
#include <BytesBuffer.h>
#include <CAStreamBasicDescription.h>
#include <CAXException.h>
#include <IceUtil/IceUtil.h>
#include <string>
#include <interf_dec.h>
#include <SP.h>
#include <HexDump.h>

#include <AudioUnit/AudioUnit.h>

#define kOutputBus 0
#define kInputBus 1



using namespace IceUtil;
using namespace std;


#define AMR_MAGIC_NUMBER "#!AMR\n"
#define TEST 1
class ProgressListener : public IceUtil::Thread
{
public:
    ProgressListener(class AudioPlayUnit_context& ref);
    virtual void run();
    void reachEnd();
private:
    Monitor<Mutex> _monitor;
    bool _wait;
    bool _destroy;
    AudioPlayUnit_context& _ref;
};

typedef Handle<ProgressListener> ProgressListenerPtr;

//------------------------------------------------------------------------------------------

class DecoderFileThread : public IceUtil::Thread {
public:
    DecoderFileThread(const char* path, BytesBufferPtr buffer);
    static size_t feedCallback(void* userData, const ChunkInfoRef,  bool terminated);
    virtual void run();
    void stop();
private: 
    string filepath;
    BytesBufferPtr _buffer;
    bool _destroy;
    FILE *file;
    BufferChunk    _cbChunk;
    void*          _decodeState;

};

typedef IceUtil::Handle<DecoderFileThread> DecoderFileThreadPtr;



#pragma -mark AudioPlayUnit_context
//---------------------------------------------------------------------------------------------------------------------------------------------------------
static int SetupRemoteIO (AudioUnit& inRemoteIOUnit, const AURenderCallbackStruct& inRenderProc, const CAStreamBasicDescription& outFormat);

typedef std::auto_ptr<PlaybackListener> PlaybackListenerPtr;
class AudioPlayUnit_context
{
public:
    friend class AudioPlayUnit;
    AudioPlayUnit_context();
    ~AudioPlayUnit_context();
    
    int initialize();

    bool start(const char* filepath);
    bool stop();
    bool passiveStop();
    bool isRunning();
    void notifyEnd();   //called in render thread
    void setPlaybackListener(const PlaybackListener& listener);
    static size_t eatCallback(void* userData, const ChunkInfoRef,  bool terminated);;
    /**
     This callback is called when the audioUnit needs new data to play through the
     speakers. If you don't have any, just don't write anything in the buffers
     */
    static OSStatus playbackCallback(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags, 
                                     const AudioTimeStamp *inTimeStamp, 
                                     UInt32 inBusNumber, 
                                     UInt32 inNumberFrames, 
                                     AudioBufferList *ioData);     

private:
    OSStatus setupBuffers();
private:
    bool _isInitialized;
	AudioComponentInstance _audioUnit;
    BytesBufferPtr _buffer;
    CAStreamBasicDescription _audioFormat;
    DecoderFileThreadPtr _decoder;
    std::string _filepath;
    ProgressListenerPtr _progressListener;
    Mutex               _mutex;
    PlaybackListenerPtr _listenerPtr;
    double               _duration;
    double               _renderstartTimestamp;
    
    string g_path;
    unsigned char _filebuffer[1<<20];
};

//---

static float parseAmrFileDuration(const string& filepath);
//------------------------------------------------------------------------------------------------------------------

AudioPlayUnit_context::AudioPlayUnit_context()
:_audioUnit(0)

{
    _isInitialized = false;
    setupBuffers();
}


AudioPlayUnit_context::~AudioPlayUnit_context()
{
}

OSStatus AudioPlayUnit_context::setupBuffers()
{
    _buffer = new BytesBuffer(2<<12);
    return noErr;
}

void AudioPlayUnit_context::notifyEnd()
{
    _progressListener->reachEnd();
}


void AudioPlayUnit_context::setPlaybackListener(const PlaybackListener& listener)
{
    _listenerPtr = PlaybackListenerPtr (new PlaybackListener(listener));
}



int AudioPlayUnit_context::initialize() {
    try {
        AURenderCallbackStruct callbackStruct;
        callbackStruct.inputProc = AudioPlayUnit_context::playbackCallback;
        callbackStruct.inputProcRefCon = this;
        _audioFormat = CAStreamBasicDescription(8000.f, 1, CAStreamBasicDescription::kPCMFormatInt16, false);
        //_audioFormat.mFormatFlags = kLinearPCMFormatFlagIsBigEndian | kLinearPCMFormatFlagIsSignedInteger;
        _audioFormat.Print();
        XThrowIfError(SetupRemoteIO(_audioUnit, callbackStruct, _audioFormat), "couldn't setup remote i/o unit");
    }
    catch (CAXException &e) {
        char buf[256];
        SP::printf( "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        return 1;
    }
    catch (...) {
        SP::printf( "An unknown error occurred\n");
        return 1;
    }
    return 0;
}




struct RenderChunk
{
    void *inRefCon;
    AudioUnitRenderActionFlags *ioActionFlags;
    const AudioTimeStamp *inTimeStamp;
    UInt32 inBusNumber;
    UInt32 inNumberFrames;
    AudioBufferList *ioData;
    OSStatus err;
} ;




size_t AudioPlayUnit_context::eatCallback(void* userData, const ChunkInfoRef info,  bool terminated)
{
    RenderChunk& _auUserData = (RenderChunk &)*userData;
    AudioPlayUnit_context* This = (AudioPlayUnit_context*)_auUserData.inRefCon;
    if (terminated && info->_data == 0) {        
        This->notifyEnd();
        return 0;
    }

    _auUserData.ioData->mNumberBuffers = This->_audioFormat.mChannelsPerFrame;     //noninterleved
    
    for (size_t i = 0; i < _auUserData.ioData->mNumberBuffers; ++i) {   //channels
        if (_auUserData.ioData->mBuffers[i].mData)     //alloc enabled
            memcpy(_auUserData.ioData->mBuffers[i].mData, info->_data, _auUserData.ioData->mBuffers[i].mDataByteSize);
        else
            _auUserData.ioData->mBuffers[i].mData = info->_data;
    }
    
    bytes2HexS((unsigned char*)_auUserData.ioData->mBuffers[0].mData, info->_size);
    //calc
    double expired = 0;
    //double lasttime = 0;
    if(This->_renderstartTimestamp == 0)
        This->_renderstartTimestamp = _auUserData.inTimeStamp->mSampleTime;
    else
        expired = _auUserData.inTimeStamp->mSampleTime - This->_renderstartTimestamp;
    
    if (This->_listenerPtr.get()) This->_listenerPtr->progress(This->_listenerPtr->userData, expired, This->_duration);
    return info->_size;
}

/**
 This callback is called when the audioUnit needs new data to play through the
 speakers. If you don't have any, just don't write anything in the buffers
 */


OSStatus AudioPlayUnit_context::playbackCallback(void *inRefCon, 
								 AudioUnitRenderActionFlags *ioActionFlags, 
								 const AudioTimeStamp *inTimeStamp, 
								 UInt32 inBusNumber, 
								 UInt32 inNumberFrames, 
								 AudioBufferList *ioData) {
#if !TEST
    // Notes: ioData contains buffers (may be more than one!)
    // Fill them up as much as you can. Remember to set the size value in each buffer to match how
    // much data is in the buffer.
    AudioPlayUnit_context* This = (AudioPlayUnit_context*)inRefCon;
    
    RenderChunk cbchunk = {0};
    cbchunk.inRefCon = This;
    cbchunk.ioActionFlags = ioActionFlags;
    cbchunk.inTimeStamp = inTimeStamp;
    cbchunk.inBusNumber = inBusNumber;
    cbchunk.inNumberFrames = inNumberFrames;
    cbchunk.ioData = ioData;
    
    BufferChunk chunk;
    chunk._callback = AudioPlayUnit_context::eatCallback;
    chunk._userData = &cbchunk;
    This->_buffer->eat(inNumberFrames*This->_audioFormat.mBytesPerFrame, &chunk);
    if (cbchunk.err) {
        SP::printf("render: error %d\n", (int)cbchunk.err);
    }
    return cbchunk.err;
#else
    
    AudioPlayUnit_context* This = (AudioPlayUnit_context*)inRefCon;
    static size_t pos = 0;
    ioData->mBuffers[0].mData = This->_filebuffer + pos;
    pos += inNumberFrames*2;
    bytes2HexS((unsigned char*)ioData->mBuffers[0].mData, inNumberFrames*2);
    return noErr;
#endif
}



bool AudioPlayUnit_context::isRunning()
{	
	OSStatus err = noErr;
	UInt32 auhalRunning = 0, size = 0;

	size = sizeof(auhalRunning);
	if(_audioUnit)
	{
		err = AudioUnitGetProperty(_audioUnit,
                                   kAudioOutputUnitProperty_IsRunning,
                                   kAudioUnitScope_Global,
                                   kOutputBus, // input element
                                   &auhalRunning,
                                   &size);
	}
    return auhalRunning;
}




bool AudioPlayUnit_context::start(const char* filepath)
{
    g_path = filepath;
    if (isRunning()) {
        return false;
    }
    try
    {
#if TEST
        FILE *fp;
        unsigned char buf[32];
        fp = fopen(filepath, "rb");
        fseek(fp, 0, SEEK_END);

        rewind(fp);
        fread(_filebuffer, 1, 6, fp);
        void* t = Decoder_Interface_init();
        size_t frame = 0;
        while (true) {
            size_t ret = fread(buf, 1, 32, fp);
            if (ret < 32) {
                break;
            }
            Decoder_Interface_Decode(t, buf, (short*)(_filebuffer + frame++*320), 1);
            
        }
        Decoder_Interface_exit(t);
        fclose(fp);

    
        XThrowIfError(initialize() , "initialize play audio unit error");
        XThrowIfError(AudioOutputUnitStart(_audioUnit), "");
        
        _duration = parseAmrFileDuration(filepath);
        if (_duration == -1) {
            return false;
        }
        if(_duration == -1) return false;
        _renderstartTimestamp = 0;
#else
        _decoder = new DecoderFileThread(filepath, _buffer);
        _decoder->start();
        _progressListener = new ProgressListener(*this);
        _progressListener->start();
        
        XThrowIfError(initialize() , "initialize play audio unit error");
        XThrowIfError(AudioOutputUnitStart(_audioUnit), "");
        
        _duration = parseAmrFileDuration(filepath);
        if (_duration == -1) {
            return false;
        }
        if(_duration == -1) return false;
        _renderstartTimestamp = 0;

#endif

    }
    catch (CAXException &e) {
		char buf[256];
		fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
		return false;
	}
	catch (...) {
		fprintf(stderr, "An unknown error occurred\n");
		return false;
	}

    return  true;
}

/**
 Stop the audioUnit, may  be called in progress thread
 */
bool AudioPlayUnit_context::stop()
{
    Mutex::Lock lock(_mutex);
    try
    {
        if (!isRunning()) {
            return false;
        }
        XThrowIfError(AudioOutputUnitStop(_audioUnit), "could not stop playback unit");
        XThrowIfError(AudioUnitUninitialize(_audioUnit), "could not uninitialize playback unit");
        XThrowIfError(AudioComponentInstanceDispose(_audioUnit), "could not Dispose playback unit");
        _buffer->terminatedEat();   //terminate eatting first
        _audioUnit= 0;
        _renderstartTimestamp = 0;
        if (_decoder.get() != NULL && _decoder->isAlive()) {
            _decoder->stop();
            _decoder->getThreadControl().join();
        }
        _buffer->terminatedFeed();      //terminate feeding last
        if (_listenerPtr.get()) _listenerPtr->finish(_listenerPtr->userData);
    }
    catch (CAXException &e) {
		char buf[256];
		fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        
		return false;
	}
	catch (...) {
		fprintf(stderr, "An unknown error occurred\n");
		return false;
	}

    return  true;
}


bool AudioPlayUnit_context::passiveStop()
{
    Mutex::Lock lock(_mutex);
    try
    {
        if (!isRunning()) {
            return false;
        }
        XThrowIfError(AudioOutputUnitStop(_audioUnit), "could not stop playback unit");
        XThrowIfError(AudioUnitUninitialize(_audioUnit), "could not uninitialize playback unit");
        XThrowIfError(AudioComponentInstanceDispose(_audioUnit), "could not Dispose playback unit");
        _buffer->terminatedEat();   //terminate eatting last
        _audioUnit= 0;
        _renderstartTimestamp = 0;
        if (_decoder.get() != NULL && _decoder->isAlive()) {
            _decoder->stop();
            _decoder->getThreadControl().join();
        }         
        if (_listenerPtr.get()) _listenerPtr->finish(_listenerPtr->userData);
    }
    catch (CAXException &e) {
		char buf[256];
		fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        
		return false;
	}
	catch (...) {
		fprintf(stderr, "An unknown error occurred\n");
		return false;
	}
    
    return  true;

}


//////////////////////////////////////////////////////////////////
AudioPlayUnit& AudioPlayUnit::instance()
{
    static AudioPlayUnit ref;
    return ref;
}

AudioPlayUnit::AudioPlayUnit()
{
    _ctx = std::auto_ptr<AudioPlayUnit_context>(new AudioPlayUnit_context);
}


bool AudioPlayUnit::startPlay(const char* filepath)
{
    return _ctx->start(filepath);
}


bool AudioPlayUnit::stopPlay()
{
    return _ctx->stop();
}

bool AudioPlayUnit::isRunning()
{
    return _ctx->isRunning();
}



AudioPlayUnit::~AudioPlayUnit()
{
    
}

void AudioPlayUnit::setPlaybackListener(const PlaybackListener& listener)
{
    return _ctx->setPlaybackListener(listener);
}

//--------------------------------------------------------------------------------------------------
int SetupRemoteIO (AudioUnit& inRemoteIOUnit, const AURenderCallbackStruct& inRenderProc, const CAStreamBasicDescription& outFormat)
{
    try {
        AudioComponentInstance& audioUnit  = inRemoteIOUnit;
        // Describe audio component
        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_RemoteIO;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        
        // Get component
        AudioComponent inputComponent = AudioComponentFindNext(NULL, &desc);
        // Get audio units
        XThrowIfError(AudioComponentInstanceNew(inputComponent, &audioUnit), "");
        
        
        // Disable IO for recording
        UInt32 flag = 0;
        XThrowIfError(AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &flag, sizeof(flag)), "could not disable record");
        flag = 1;
        // Enable IO for playback
        XThrowIfError(AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &flag, sizeof(flag)), "could not enable play");
        
        //play format
        XThrowIfError(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, kOutputBus, &outFormat, sizeof(outFormat)), "couldn't set play format");
        // Set output callback
        XThrowIfError(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, kOutputBus, &inRenderProc, sizeof(AURenderCallbackStruct)) , "Could not setRender callback");
        flag = 0;
        XThrowIfError(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_ShouldAllocateBuffer, kAudioUnitScope_Input, kOutputBus, &flag, sizeof(flag)), "Could not disable buffer allocation for the player");
        // Initialise
        XThrowIfError(AudioUnitInitialize(audioUnit), "could not init audio unit");

    }
    catch (CAXException &e) {
		char buf[256];
		fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        
		return 1;
	}
	catch (...) {
		fprintf(stderr, "An unknown error occurred\n");
		return 1;
	}
    return 0;
}




//---------------------------------------------------------------------------------------------------------


//amr 11.20
 float parseAmrFileDuration(const string& filepath)
{
    char buffer[32];
    size_t ret = 0;
    FILE* fp = fopen(filepath.c_str(), "rb");
    if (!fp) return -1;
        
    
    ret = fread(buffer, 1, 6, fp);
    if (ret == 0)  return -1;
    //verify file format
    if (0 != strncmp(AMR_MAGIC_NUMBER, buffer, 6) ) {
        return -1;
    }
    //verify bitrate
    ret = fread(buffer, 1, 1, fp);
    if (ret == 0)   return -1;
    if (7 != (buffer[0] & 0x78) >> 3  )
        return -1;
    fseek(fp, -1L, SEEK_END);
    size_t sz = ftell(fp);
    float duration = round(sz/320.f + 0.5);
    fclose(fp);
    return duration;
}





DecoderFileThread::DecoderFileThread(const char* path, BytesBufferPtr buffer)
: filepath(path)
, _buffer(buffer)
, _destroy(false)
, _decodeState(0)

, file(0)
{
    _cbChunk._userData = this;
    _cbChunk._callback = DecoderFileThread::feedCallback;
}

void DecoderFileThread::run()
{
    _decodeState = Decoder_Interface_init();
    file = fopen(filepath.c_str(), "rb");
    fseek(file, 6, 0);
    do {
        _buffer->feed(160*2, &_cbChunk);
    } while (!_destroy);
    Decoder_Interface_exit(_decodeState);
    fclose(file);
    //SP::printf("\nfinish decode file\n");
}

void DecoderFileThread::stop()
{
    _destroy = true;
}



size_t DecoderFileThread::feedCallback(void* userData, const ChunkInfoRef info,  bool terminated)
{
    static unsigned char buffer[32];
    DecoderFileThread* This = (DecoderFileThread*)userData;
    
    if (terminated) {
        This->stop();
        return 0;
    }
    
    while (true){
        size_t ret;
        ret = fread(buffer, 1, 1, This->file);
        if(ret < 1) {
            This->_buffer->terminatedFeed();    //feeding terminated first
            This->stop();
            return 0;
        }
        //err data, skip to next frame
        if (7 != ((buffer[0] & 0x78) >> 3) ) {
            continue;
        }
        //read the data
        ret = fread(buffer+1, 1, 31, This->file);
        if(ret < 31) {
            This->_buffer->terminatedFeed();    //feeding terminated first
            This->stop();
            return 0;
        }
        break;
    }
    
    Decoder_Interface_Decode(This->_decodeState, buffer, (short*)info->_data, 1);
    return 160*2;
}


//---------------------------------------------------------------------------------------------------------------------------------------

ProgressListener::ProgressListener(AudioPlayUnit_context& ref)
:_ref(ref)
,_wait(true)
,_destroy(false)
{
    
}

void ProgressListener::reachEnd()
{
    Monitor<Mutex>::Lock lock(_monitor);
    _wait = false;
    _monitor.notify();
}

void ProgressListener::run()
{
    {
        Monitor<Mutex>::Lock lock(_monitor);
        while (_wait && !_destroy) {
            _monitor.wait();
        }
    }
    if (!_wait) {
        _ref.passiveStop();
    }    
}


