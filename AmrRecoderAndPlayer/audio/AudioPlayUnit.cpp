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


#define kOutputBus 0
#define kInputBus 1



using namespace IceUtil;
using namespace std;


#define AMR_MAGIC_NUMBER "#!AMR\n"



class DecoderFileThread : public IceUtil::Thread {
public:
    DecoderFileThread(const char* path, BytesBufferPtr buffer);
    static size_t feedCallback(void* userData, const ChunkInfoRef,  bool terminated);
    virtual void run();
    void stop();
    bool parseAmrFileFormat();
private: 
    string filepath;
    BytesBufferPtr _buffer;
    bool _destroy;
    FILE *file;
    BufferChunk    _cbChunk;
    void* _decodeState;

};

typedef IceUtil::Handle<DecoderFileThread> DecoderFileThreadPtr;

//amr 11.20
bool DecoderFileThread::parseAmrFileFormat()
{
    char buffer[32];
    size_t ret = 0;
    FILE* fp = fopen(filepath.c_str(), "rb");
    ret = fread(buffer, 1, 6, fp);
    if (ret == 0)  return false;
    //verify file format
    if (0 != strncmp(AMR_MAGIC_NUMBER, buffer, 6) ) {
        //suppose 3gp file wrapper, find the amr data start postion;
        return false;
    }
    //verify bitrate
    ret = fread(buffer, 1, 1, fp);
    if (ret == 0)   return false;
    
    
    if (7 != (buffer[0] & 0x78) >> 3  )
        return false;
    fclose(fp);
    return true;
}



size_t DecoderFileThread::feedCallback(void* userData, const ChunkInfoRef info,  bool terminated)
{
    static unsigned char buffer[32];
    DecoderFileThread* This = (DecoderFileThread*)userData;
    
    if (terminated) {
        This->_destroy = true;
        return 0;
    }
    
    
    while (true){
        size_t ret;
        ret = fread(buffer, 1, 1, This->file);
        if(ret < 1) {
            This->_destroy = true;
            This->_buffer->terminatedFeed();
            return 0;
        }
        //err data, skip to next frame
        if (7 != ((buffer[0] & 0x78) >> 3) ) {
            continue;
        }
        //read the data
        ret = fread(buffer+1, 1, 31, This->file);
        if(ret < 31) {
            This->_destroy = true;
            This->_buffer->terminatedFeed();
            return 0;
        }
        break;
    }
    bytes2HexS(buffer, 32);
    Decoder_Interface_Decode(This->_decodeState, buffer, (short*)info->_data, 1);

    return 160*2;
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
    SP::printf("finish decode file");
}


#pragma -mark AudioPlayUnit_context
//---------------------------------------------------------------------------------------------------------------------------------------------------------
static int SetupRemoteIO (AudioUnit& inRemoteIOUnit, const AURenderCallbackStruct& inRenderProc, const CAStreamBasicDescription& outFormat);


class AudioPlayUnit_context
{
public:
    friend class AudioPlayUnit;
    AudioPlayUnit_context();
    ~AudioPlayUnit_context();
    
    void initialize(float sampleRate, int channel, int sampleDeep);
    void uninitialize();
    bool isInitialized();
    OSStatus start(const char* filepath);
    OSStatus stop();
    bool isRunning();
    static void rioInterruptionListener(void *inClientData, UInt32 inInterruptionState);
    static void propListener(	void *                  inClientData,
                                             AudioSessionPropertyID	inID,
                                             UInt32                  inDataSize,
                      const void *            inData);
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
};


AudioPlayUnit_context::AudioPlayUnit_context()
:_audioUnit(0)
{
    _isInitialized = false;
    setupBuffers();
}


AudioPlayUnit_context::~AudioPlayUnit_context()
{
    uninitialize();
}

OSStatus AudioPlayUnit_context::setupBuffers()
{
    _buffer = new BytesBuffer(2<<12);
    return noErr;
}

bool AudioPlayUnit_context::isInitialized()
{
    return _isInitialized;
}

void AudioPlayUnit_context::uninitialize()
{
    if (_isInitialized) 
    {
        AudioUnitUninitialize(_audioUnit);
        _isInitialized = false;        
    }
}


void AudioPlayUnit_context::rioInterruptionListener(void *inClientData, UInt32 inInterruptionState)
{
    printf ("Interrupted! inInterruptionState=%ld\n", inInterruptionState);
    AudioPlayUnit_context *This = (AudioPlayUnit_context*)inClientData;
    
    switch (inInterruptionState) {
        case kAudioSessionBeginInterruption:
            //shutdown audio unit
            break;
        case kAudioSessionEndInterruption:
            //            CheckError(AudioQueueStart(appDelegate.audioQueue, 0), \
            "Couldn't restart the audio queue");
            break;
        default:
            break;
    };
}



void AudioPlayUnit_context::propListener(	void *                  inClientData,
                                          AudioSessionPropertyID	inID,
                                          UInt32                  inDataSize,
                                          const void *            inData)
{
    
}

void AudioPlayUnit_context::initialize(float sampleRate, int channel, int sampleDeep) {
    if(_isInitialized)
        return;
    
    try {
#if 0
        // Initialize and configure the audio session
        XThrowIfError(AudioSessionInitialize(NULL, NULL, AudioPlayUnit_context::rioInterruptionListener, this), "couldn't initialize audio session for playback");
        
        UInt32 audioCategory = kAudioSessionCategory_MediaPlayback;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory), "couldn't set audio category");
        XThrowIfError(AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, propListener, this), "couldn't set property listener");
//        
        Float32 preferredBufferSize = .02;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");
//
        Float64 hwSampleRate;
        UInt32 size = sizeof(hwSampleRate);
        XThrowIfError(AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &size, &hwSampleRate), "couldn't get hw sample rate");
        
        XThrowIfError(AudioSessionSetActive(true), "couldn't set audio session active\n");
#endif
        
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
        ///return 1;
    }
    catch (...) {
        SP::printf( "An unknown error occurred\n");
        //return 1;
    }	
    _isInitialized = true;    
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
        This->stop();
        return 0;
    }

    _auUserData.ioData->mNumberBuffers = This->_audioFormat.mChannelsPerFrame;     //noninterleved
    
    for (size_t i = 0; i < _auUserData.ioData->mNumberBuffers; ++i) {   //channels
        if (_auUserData.ioData->mBuffers[i].mData) {    //alloc
            memcpy(_auUserData.ioData->mBuffers[i].mData, info->_data, _auUserData.ioData->mBuffers[i].mDataByteSize);
        } else {
            _auUserData.ioData->mBuffers[i].mData = info->_data;
        }
    }
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




OSStatus AudioPlayUnit_context::start(const char* filepath)
{
    initialize(0, 0, 0);
    _decoder = new DecoderFileThread(filepath, _buffer);
    assert(_decoder->parseAmrFileFormat());
    _decoder->start();
    
    try
    {
        XThrowIfError(AudioOutputUnitStart(_audioUnit), "");
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
    return  0;
}

/**
 Stop the audioUnit
 */
OSStatus AudioPlayUnit_context::stop()
{
    try
    {
        XThrowIfError(AudioOutputUnitStop(_audioUnit), "could not stop playback unit");
        XThrowIfError(AudioUnitUninitialize(_audioUnit), "could not uninitialize playback unit");
        XThrowIfError(AudioComponentInstanceDispose(_audioUnit), "could not Dispose playback unit");
        _audioUnit= 0;

//        XThrowIfError(AudioSessionSetActive(false), "couldn't set audio session deactive\n");
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

    return  0;
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

void AudioPlayUnit::initialize(float sampleRate, int channel, int sampleDeep)
{
    _ctx->initialize(sampleRate, channel, sampleDeep);    
}


void AudioPlayUnit::uninitialize()
{
    _ctx->uninitialize();
}

bool AudioPlayUnit::isInitialized()
{
    return _ctx->isInitialized();
}


OSStatus AudioPlayUnit::startPlay(const char* filepath)
{
    return _ctx->start(filepath);
}


OSStatus AudioPlayUnit::stopPlay()
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

