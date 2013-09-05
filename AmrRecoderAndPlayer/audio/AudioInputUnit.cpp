#include "AudioInputUnit.h"
#include <AudioToolbox/AudioToolbox.h>
#include <BytesBuffer.h>
#include <IceUtil/IceUtil.h>
#include <SP.h>
#include <interf_enc.h>
#include <stdio.h>
#include <CAStreamBasicDescription.h>
#include <CAXException.h>
#include <HexDump.h>
#include <interf_dec.h>
#define kOutputBus 0 /*bus 0 represents a stream to output hardware*/
#define kInputBus 1  /*bus 1 represents a stream from input hardware*/

#define AMR_MAGIC_NUMBER "#!AMR\n"

typedef struct AUUserData{
    void *inRefCon;
    AudioUnitRenderActionFlags *ioActionFlags;
    AudioTimeStamp *inTimeStamp;
    UInt32 inBusNumber;
    UInt32 inNumberFrames;
    AudioBufferList * ioData;
    OSStatus err;
    
} *AUUserDataRef;

static int SetupRemoteIO (AudioUnit& inRemoteIOUnit, const AURenderCallbackStruct&, const CAStreamBasicDescription& );
//-----------------------------------------------------------------------------------------------------------------------------------------------------------
class EncodeThread : public IceUtil::Thread
{
public:
    EncodeThread(const char* filepath, BytesBufferPtr buffer);
    virtual ~EncodeThread();
    static size_t callBackFun(void* userData, const ChunkInfoRef,  bool terminated);
    
    void stop();
    void waitForCompleted();
    void cancel();
    virtual void run();
    
private:
    //PopBufferChunk  chunk;
    std::string _filepath;
    void * armEncodeState;
    bool  _destroy;
    bool  _cancel;
    BytesBufferPtr _buffer;
    BufferChunk    _cbChunk;
    unsigned char  _armFrame[32];
    FILE *file;
    
    void *_amrDecodeState;
    short _amrDecodeFrame[160];
};

typedef IceUtil::Handle<EncodeThread> EncodeThreadPtr;



//-----------------------------------------------------------------------------------------------------------------------------------------------------------


//#define RECODESTREAM

typedef std::auto_ptr<RecordListener> RecordListenerPtr;
class AudioInputUnit_context
{
public:
	static void rioInterruptionListener(void *inClientData, UInt32 inInterruptionState);
    static void propListener(	void *                  inClientData,
                             AudioSessionPropertyID	inID,
                             UInt32                  inDataSize,
                             const void *            inData);
    friend class AudioInputUnit;
    AudioInputUnit_context();
    ~AudioInputUnit_context();
    int initialize();
    
    bool start(const char* path);
    bool stop();
    bool cancel();
    bool isRunning();
    void setRecordListener(const RecordListener& listener);
    static OSStatus recordingCallback(void *inRefCon,
                                      AudioUnitRenderActionFlags *ioActionFlags,
                                      const AudioTimeStamp *inTimeStamp,
                                      UInt32 inBusNumber,
                                      UInt32 inNumberFrames,
                                      AudioBufferList *ioData);
    
    static size_t feedCallBackFun(void* userData, const ChunkInfoRef,  bool terminated);
private:
    void setupBuffers();
    inline void makeBufferSilent (AudioBufferList * ioData);
private:
    BytesBufferPtr _buffer;
	AudioComponentInstance _audioUnit;
    CAStreamBasicDescription _audioFormat;
    BufferChunk chunk;
    
    std::string filepath;
    EncodeThreadPtr _encoder;
    RecordListenerPtr _listener;
    double      _renderstartTimestamp;
    size_t      _expired;
};


AudioInputUnit_context::AudioInputUnit_context()
:_audioUnit(0)
{
    setupBuffers();
}


AudioInputUnit_context::~AudioInputUnit_context()
{
    
}


void AudioInputUnit_context::propListener(	void *                  inClientData,
                                          AudioSessionPropertyID	inID,
                                          UInt32                  inDataSize,
                                          const void *            inData)
{
    
}



int AudioInputUnit_context::initialize() {
    
    try {
        AURenderCallbackStruct callbackStruct;
        callbackStruct.inputProc = recordingCallback;
        callbackStruct.inputProcRefCon = this;
        _audioFormat = CAStreamBasicDescription(8000, 1, CAStreamBasicDescription::kPCMFormatInt16, false);
        XThrowIfError(SetupRemoteIO(_audioUnit, callbackStruct, _audioFormat), "couldn't setup remote i/o unit");
    } catch(CAXException &e) {
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




bool AudioInputUnit_context::isRunning()
{
	OSStatus err = noErr;
	UInt32 auhalRunning = 0, size = 0;
    
	size = sizeof(auhalRunning);
	if(_audioUnit)
	{
		err = AudioUnitGetProperty(_audioUnit,
                                   kAudioOutputUnitProperty_IsRunning,
                                   kAudioUnitScope_Global,
                                   kInputBus, // input element
                                   &auhalRunning,
                                   &size);
	}
    return auhalRunning;
}





size_t AudioInputUnit_context::feedCallBackFun(void* userData, const ChunkInfoRef info,  bool terminated)
{
    
    AUUserData& _auUserData = (AUUserData&)*userData;
    AudioInputUnit_context *This = (AudioInputUnit_context*)_auUserData.inRefCon;
    
    // Allocate an AudioBufferList plus enough space for
    // array of AudioBuffers
    UInt32 propsize = offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) *
                                                                This->_audioFormat.mChannelsPerFrame);
    _auUserData.ioData = (AudioBufferList *)malloc(propsize);
    _auUserData.ioData->mNumberBuffers =  This->_audioFormat.mChannelsPerFrame;     //noninterleved
    
    for (size_t i = 0; i < _auUserData.ioData->mNumberBuffers; ++i) {   //channels
        _auUserData.ioData->mBuffers[i].mNumberChannels = 1;
        _auUserData.ioData->mBuffers[i].mDataByteSize = info->_size/_auUserData.ioData->mNumberBuffers;
        _auUserData.ioData->mBuffers[i].mData = info->_data + i*info->_size/_auUserData.ioData->mNumberBuffers;
    }
    
    //Get the new audio data
    
	_auUserData.err = AudioUnitRender(This->_audioUnit,
                                      _auUserData.ioActionFlags,
                                      _auUserData.inTimeStamp,
                                      _auUserData.inBusNumber,
                                      _auUserData.inNumberFrames, /* of frames requested*/
                                      _auUserData.ioData );/* Audio Buffer List to hold data*/
    return info->_size;
}

OSStatus AudioInputUnit_context::recordingCallback(void *inRefCon,
                                                   AudioUnitRenderActionFlags *ioActionFlags,
                                                   const AudioTimeStamp *inTimeStamp,
                                                   UInt32 inBusNumber,
                                                   UInt32 inNumberFrames,
                                                   AudioBufferList * ioData)
{
    
	AudioInputUnit_context *This = (AudioInputUnit_context *)inRefCon;
    
    static AUUserData _auUserData = {0};
    _auUserData.inRefCon = This;
    _auUserData.ioActionFlags = ioActionFlags;
    _auUserData.inTimeStamp = (AudioTimeStamp*)inTimeStamp;
    _auUserData.inBusNumber = inBusNumber;
    _auUserData.inNumberFrames = inNumberFrames;
    _auUserData.ioData = ioData;
    _auUserData.err = noErr;
    
    This->chunk._callback = AudioInputUnit_context::feedCallBackFun;
    This->chunk._userData =&_auUserData;
    This->_buffer->feed(inNumberFrames * This->_audioFormat.mBytesPerPacket, &This->chunk);
    if (_auUserData.err) { SP::printf("render: error %d\n", (int)_auUserData.err);}
    
    
    if(This->_renderstartTimestamp == 0)
    {
        This->_renderstartTimestamp = IceUtil::Time::now().toMilliSeconds();
        This->_expired = 0;
    }
    else
        This->_expired = IceUtil::Time::now().toMilliSeconds() - This->_renderstartTimestamp;
    if (This->_listener.get()) This->_listener->progress(This->_listener->userData, This->_expired);
	return _auUserData.err;
}



void AudioInputUnit_context::setupBuffers()
{
    _buffer = new BytesBuffer(2<<12);
}



bool AudioInputUnit_context::start(const char* path)
{
    try
    {
        XThrowIfError(initialize(), "could not initialize record unit");
        XThrowIfError(AudioOutputUnitStart(_audioUnit), "could not start record unit");
        _renderstartTimestamp = 0;
        _encoder = new EncodeThread(path, _buffer);
        _encoder->start();
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
 Stop the audioUnit
 */
bool AudioInputUnit_context::stop()
{
    try {
        if (!isRunning()) return true;
        XThrowIfError(AudioOutputUnitStop(_audioUnit), "couldn't stop record audio unit");
        XThrowIfError(AudioUnitUninitialize(_audioUnit), "couldn't uninitialize record audio unit");
        XThrowIfError(AudioComponentInstanceDispose(_audioUnit), "could not Dispose record unit");
        _audioUnit = 0;
        _renderstartTimestamp = 0;
        _buffer->terminatedFeed();
        
        _encoder->stop();
        
        _encoder->getThreadControl().join();
        _buffer->terminatedEat();
        if (_listener.get()) _listener->finish(_listener->userData, _expired);
    } catch(CAXException &e) {
        char buf[256];
        SP::printf("Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        return false;
    }
    catch (...) {
		SP::printf("An unknown error occurred\n");
        return false;
	}
    return true;
}

bool AudioInputUnit_context::cancel()
{
    try {
        if (!isRunning()) return true;
        
        XThrowIfError(AudioSessionRemovePropertyListenerWithUserData(kAudioSessionProperty_AudioRouteChange, propListener, this), "could not remove PropertyListener");
        XThrowIfError(AudioOutputUnitStop(_audioUnit), "couldn't stop record audio unit");
        XThrowIfError(AudioUnitUninitialize(_audioUnit), "couldn't uninitialize record audio unit");
        XThrowIfError(AudioComponentInstanceDispose(_audioUnit), "could not Dispose record unit");
        _audioUnit = 0;
        
        _renderstartTimestamp = 0;
        
        _encoder->cancel();     //cancel flag set first
        _buffer->terminatedFeed();
        
        _encoder->getThreadControl().join();
        _buffer->terminatedEat();
    } catch(CAXException &e) {
        char buf[256];
        SP::printf("Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        return false;
    }
    catch (...) {
		SP::printf("An unknown error occurred\n");
        return false;
	}
    return true;
}

void AudioInputUnit_context::setRecordListener(const RecordListener& listener)
{
    _listener = RecordListenerPtr(new RecordListener(listener));
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------




AudioInputUnit& AudioInputUnit::instance()
{
    static AudioInputUnit ref;
    return ref;
}

AudioInputUnit::AudioInputUnit()
{
    _ctx = std::auto_ptr< AudioInputUnit_context>(new AudioInputUnit_context);
}


bool AudioInputUnit::start(const char* path)
{
    return _ctx->start(path);
}


bool AudioInputUnit::stop()
{
    return _ctx->stop();
}

bool AudioInputUnit::cancel()
{
    return _ctx->cancel();
}

void AudioInputUnit::setRecordListener(const RecordListener& listener)
{
    _ctx->setRecordListener(listener);
}

AudioInputUnit::~AudioInputUnit()
{
    
}

int SetupRemoteIO (AudioUnit& inRemoteIOUnit, const AURenderCallbackStruct& inRenderProc, const CAStreamBasicDescription& outFormat)
{
	try {
		// Open the output unit
		AudioComponentDescription desc;
		desc.componentType = kAudioUnitType_Output;
		desc.componentSubType = kAudioUnitSubType_RemoteIO;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;
		
		AudioComponent comp = AudioComponentFindNext(NULL, &desc);
		
		XThrowIfError(AudioComponentInstanceNew(comp, &inRemoteIOUnit), "couldn't open the remote I/O unit");
        
		UInt32 one = 1;
		XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &one, sizeof(one)), "couldn't enable record on the remote I/O unit");
        one = 0;
        XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &one, sizeof(one)), "couldn't disable play on the remote I/O unit");
        
        XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioOutputUnitProperty_SetInputCallback , kAudioUnitScope_Input, kInputBus, &inRenderProc, sizeof(inRenderProc)), "couldn't set remote i/o render callback");
		XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, kInputBus, &outFormat, sizeof(outFormat)), "couldn't set the remote I/O unit's input client format");
        // Disable buffer allocation for the recorder (optional - do this if we want to pass in our own)
        UInt32 flag = 0;
        XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_ShouldAllocateBuffer, kAudioUnitScope_Output, kInputBus, &flag, sizeof(flag)), "Could not disable buffer allocation for the recorder");
		XThrowIfError(AudioUnitInitialize(inRemoteIOUnit), "couldn't initialize the remote I/O unit");
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

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
size_t EncodeThread::callBackFun(void* userData, const ChunkInfoRef info,  bool terminated)
{
    EncodeThread *This = (EncodeThread*)userData;
    if (info->_data == 0 && terminated) {
        This->stop();
        //SP::printf("\nnomore data, quit record\n");
        return 0;
    }
    if (info->_size < 160*2) {
        This->stop();
        return info->_size;
    }
    int ret = Encoder_Interface_Encode(This->armEncodeState, MR122, (const short*)info->_data, This->_armFrame, 0);
    fwrite(This->_armFrame, sizeof(unsigned char), ret, This->file);
    return  info->_size;
}


EncodeThread::EncodeThread(const char* filepath, BytesBufferPtr buffer)
:_filepath(filepath)
,_buffer(buffer)
,file(0)
,_destroy(false)
,_cancel(false)
{
    _cbChunk._callback = EncodeThread::callBackFun;
    _cbChunk._userData = this;
}

void EncodeThread::run()
{
    
    int dtx = 0;
    armEncodeState = Encoder_Interface_init(dtx);
    file = fopen(_filepath.c_str(), "wb+");
    fwrite(AMR_MAGIC_NUMBER, sizeof(char), 6, file);
    do {
        _buffer->eat(160*2, &_cbChunk);
    } while (!_destroy && !_cancel);
    Encoder_Interface_exit(armEncodeState);
    
    fclose(file);
    if (_cancel) {
        ::remove(_filepath.c_str());
        SP::printf("remove file\n");
    } else
        SP::printf("\nsave file\n");
}

EncodeThread::~EncodeThread()
{
    
}

void EncodeThread::waitForCompleted()
{
    
}

void EncodeThread::stop()
{
    if (!_destroy) {
        _destroy = true;
        
    }
}

void EncodeThread::cancel()
{
    if (!_cancel) {
        _cancel = true;
    }
}



//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

void processBuffer(AudioBufferList* audioBufferList, float gain)
{
    //    AudioBuffer sourceBuffer = audioBufferList->mBuffers[0];
    
    // we check here if the input data byte size has changed
    //	if (audioBuffer.mDataByteSize != sourceBuffer.mDataByteSize) {
    //        // clear old buffer
    //		free(audioBuffer.mData);
    //        // assing new byte size and allocate them on mData
    //		audioBuffer.mDataByteSize = sourceBuffer.mDataByteSize;
    //		audioBuffer.mData = malloc(sourceBuffer.mDataByteSize);
    //	}
    
    /**
     Here we modify the raw data buffer now.
     In my example this is a simple input volume gain.
     iOS 5 has this on board now, but as example quite good.
     */
    
    for (int i = 0; audioBufferList->mNumberBuffers; ++i) {
        // loop over every packet
        for (int nb = 0; nb < (audioBufferList->mBuffers[i].mDataByteSize / 2); nb++) {
            short *editBuffer = (short*)audioBufferList->mBuffers[i].mData;
            // we check if the gain has been modified to save resoures
            if (gain != 0) {
                // we need more accuracy in our calculation so we calculate with doubles
                double gainSample = ((double)editBuffer[nb]) / 32767.0;
                
                /*
                 at this point we multiply with our gain factor
                 we dont make a addition to prevent generation of sound where no sound is.
                 
                 no noise
                 0*10=0
                 
                 noise if zero
                 0+10=10
                 */
                gainSample *= gain;
                
                /**
                 our signal range cant be higher or lesser -1.0/1.0
                 we prevent that the signal got outside our range
                 */
                gainSample = (gainSample < -1.0) ? -1.0 : (gainSample > 1.0) ? 1.0 : gainSample;
                
                /*
                 This thing here is a little helper to shape our incoming wave.
                 The sound gets pretty warm and better and the noise is reduced a lot.
                 Feel free to outcomment this line and here again.
                 
                 You can see here what happens here http://silentmatt.com/javascript-function-plotter/
                 Copy this to the command line and hit enter: plot y=(1.5*x)-0.5*x*x*x
                 */
                
                gainSample = (1.5 * gainSample) - 0.5 * gainSample * gainSample * gainSample;
                
                // multiply the new signal back to short
                gainSample = gainSample * 32767.0;
                
                // write calculate sample back to the buffer
                editBuffer[nb] = (SInt16)gainSample;
            }
        }
    }
}

