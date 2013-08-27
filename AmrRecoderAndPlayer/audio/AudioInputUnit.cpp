#include "AudioInputUnit.h"
#import <AudioToolbox/AudioToolbox.h>
#include <BytesBuffer.h>
#include <IceUtil/IceUtil.h>
#include <SafePrinter.h>
//#include <CoreAudio/CoreAudio.h>

//#include "CAStreamBasicDescription.h"
#define kOutputBus 0
#define kInputBus 1

#define checkErr( err) \
if(err) {\
OSStatus error = static_cast<OSStatus>(err);\
fprintf(stdout, "CAPlayThrough Error: %ld ->  %s:  %d\n",  error,\
__FILE__, \
__LINE__\
);\
fflush(stdout);\
return err; \
}         
extern SafePrinterPtr g_p;

void CheckError(OSStatus error, const char *operation)
{
    if (error == noErr) return;
    
    char errorString[20];
    // See if it appears to be a 4-char-code
    *(UInt32 *)(errorString + 1) = CFSwapInt32HostToBig(error);
    if (isprint(errorString[1]) && isprint(errorString[2]) &&
        isprint(errorString[3]) && isprint(errorString[4])) {
        errorString[0] = errorString[5] = '\'';
        errorString[6] = '\0';
    } else
        // No, format it as an integer
        sprintf(errorString, "%d", (int)error);
    
    fprintf(stderr, "Error: %s (%s)\n", operation, errorString);
    
    exit(1);
}


void checkStatus(int status){
	if (status) {
		printf("Status not 0! %d\n", status);
        //		exit(1);
	}
}

//#define RECODESTREAM
class AudioInputUnit_context
{
public:
    friend class AudioInputUnit;
    AudioInputUnit_context();
    ~AudioInputUnit_context();
    OSStatus initAudioInput();
    void initialize(float sampleRate, int channel, int sampleDeep);
    void uninitialize();
    bool isInitialized();
    OSStatus start();
    OSStatus stop();
    bool isRunning();
    static OSStatus recordingCallback(void *inRefCon,
                             AudioUnitRenderActionFlags *ioActionFlags, 
                             const AudioTimeStamp *inTimeStamp,
                             UInt32 inBusNumber, 
                             UInt32 inNumberFrames, 
                             AudioBufferList *ioData);
    
    static size_t feedCallBackFun(void* userData, const ChunkInfoRef,  bool terminated);
private:
    OSStatus enableIO();
    OSStatus callbackSetup();
    OSStatus setupFomart();
    OSStatus setupBuffers();
    inline void makeBufferSilent (AudioBufferList * ioData);
private:
//    std::auto_ptr<RingBufferA> _ring;
    BytesBufferPtr _buffer;
	AudioComponentInstance _audioUnit;
	AudioBufferList _inputBuffer;   // this will hold the latest data from the microphone   
    int             _isInitialized;
    AudioStreamBasicDescription audioFormat;
    BufferChunk chunk;
#ifdef RECODESTREAM
    FILE* _recodestreamfile;
#endif    
};


AudioInputUnit_context::AudioInputUnit_context()
{
    _isInitialized = false;
    setupBuffers();
}


AudioInputUnit_context::~AudioInputUnit_context()
{
    uninitialize();
}


void AudioInputUnit_context::uninitialize()
{
    if(_isInitialized)        
    {
        AudioUnitUninitialize(_audioUnit);   
        _isInitialized = false;
    }
}

bool AudioInputUnit_context::isInitialized()
{
    return _isInitialized;
}

void AudioInputUnit_context::initialize(float sampleRate, int channel, int sampleDeep) {
    if (_isInitialized) 
        return;

	AudioComponentInstance& audioUnit  = _audioUnit;
	OSStatus status;
	
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
	status = AudioComponentInstanceNew(inputComponent, &audioUnit);
	checkStatus(status);
	
	// Enable IO for recording
	UInt32 flag = 1;
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioOutputUnitProperty_EnableIO, 
								  kAudioUnitScope_Input, 
								  kInputBus,
								  &flag, 
								  sizeof(flag));
	checkStatus(status);
	
    
    
    
    flag = 0;
	// Enable IO for playback
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioOutputUnitProperty_EnableIO, 
								  kAudioUnitScope_Output, 
								  kOutputBus,
								  &flag, 
								  sizeof(flag));
	checkStatus(status);
	
	// Describe format
	
	audioFormat.mSampleRate			= sampleRate;
	audioFormat.mFormatID			= kAudioFormatLinearPCM;
	audioFormat.mFormatFlags		= kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsAlignedHigh | kAudioFormatFlagIsPacked;
	
	audioFormat.mChannelsPerFrame	= channel;
	audioFormat.mBitsPerChannel		= sampleDeep;    
    audioFormat.mFramesPerPacket	= 1;
	audioFormat.mBytesPerPacket		= audioFormat.mFramesPerPacket*sampleDeep/8;
	audioFormat.mBytesPerFrame		= channel*audioFormat.mFramesPerPacket*sampleDeep/8;
	
	// Apply format
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioUnitProperty_StreamFormat, 
								  kAudioUnitScope_Output, 
								  kInputBus, 
								  &audioFormat, 
								  sizeof(audioFormat));
	checkStatus(status);
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioUnitProperty_StreamFormat, 
								  kAudioUnitScope_Input, 
								  kOutputBus, 
								  &audioFormat, 
								  sizeof(audioFormat));
	checkStatus(status);
	
	
	// Set input callback
	AURenderCallbackStruct callbackStruct;
	callbackStruct.inputProc = recordingCallback;
	callbackStruct.inputProcRefCon = this;
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioOutputUnitProperty_SetInputCallback, 
								  kAudioUnitScope_Global, 
								  kInputBus, 
								  &callbackStruct, 
								  sizeof(callbackStruct));
	checkStatus(status);
	

	// Disable buffer allocation for the recorder (optional - do this if we want to pass in our own)
	flag = 0;
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioUnitProperty_ShouldAllocateBuffer,
								  kAudioUnitScope_Output, 
								  kInputBus,
								  &flag, 
								  sizeof(flag));
	
	// Allocate our own buffers (1 channel, 16 bits per sample, thus 16 bits per frame, thus 2 bytes per frame).
	// Practice learns the buffers used contain 512 frames, if this changes it will be fixed in processAudio.

	
	// Initialise
	status = AudioUnitInitialize(audioUnit);
	checkStatus(status); 
    
    _isInitialized = true;
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
	
    checkErr(err);
    return auhalRunning;
}


typedef struct AUUserData{
    void *inRefCon;
    AudioUnitRenderActionFlags *ioActionFlags;
    AudioTimeStamp *inTimeStamp;
    UInt32 inBusNumber;
    UInt32 inNumberFrames;
    AudioBufferList * ioData;
} *AUUserDataRef;


size_t AudioInputUnit_context::feedCallBackFun(void* userData, const ChunkInfoRef info,  bool terminated)
{
    AUUserDataRef ref = (AUUserDataRef)userData;
    AudioInputUnit_context *This = (AudioInputUnit_context*)ref->inRefCon;
    This->_inputBuffer.mBuffers[0].mNumberChannels = 1;
    // Put buffer in a AudioBufferList
    //    unsigned limitedSize;
	This->_inputBuffer.mNumberBuffers = 1;
	This->_inputBuffer.mBuffers[0].mNumberChannels = 1;
	This->_inputBuffer.mBuffers[0].mDataByteSize = info->_size;
    This->_inputBuffer.mBuffers[0].mData = info->_data;
    //Get the new audio data
    OSStatus err = noErr;
	err = AudioUnitRender(This->_audioUnit,
                          ref->ioActionFlags,
                          ref->inTimeStamp,
                          ref->inBusNumber,
                          ref->inNumberFrames, //# of frames requested
                          &This->_inputBuffer );// Audio Buffer List to hold data
	CheckError(err, "AudioInputUnit_context::InputProc");
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
    g_p->printf("recordingCallback %d  %d %x\n", inBusNumber, inNumberFrames, ioData);
    AUUserDataRef ref = new AUUserData();
    ref->inRefCon = inRefCon;
    ref->ioActionFlags = ioActionFlags;
    ref->inBusNumber = inBusNumber;
    ref->inNumberFrames = inNumberFrames;
    ref->ioData = ioData;
    
    
    This->chunk._callback = AudioInputUnit_context::feedCallBackFun;
    This->chunk._userData = ref;
    This->_buffer->feed(inNumberFrames * 2, &This->chunk);


//	This->_inputBuffer.mBuffers[0].mData = 
//
//    if (0 == This->_inputBuffer.mBuffers[0].mData)
//    {
//        printf("input get buffer expired!! direct return\n");
//        return err;
//    }
//    //else 
//    
//
    
	//Get the new audio data
//	err = AudioUnitRender(This->_audioUnit,
//                          ioActionFlags,
//                          inTimeStamp, 
//                          inBusNumber,     
//                          inNumberFrames, //# of frames requested
//                          &This->_inputBuffer );// Audio Buffer List to hold data
//	CheckError(err, "AudioInputUnit_context::InputProc");
//
//#ifdef RECODESTREAM
//    fwrite(This->_inputBuffer.mBuffers[0].mData, limitedSize, 1, This->_recodestreamfile);
//#endif    
//    This->_ring->put(limitedSize);
	return noErr;
}


//
//
//OSStatus AudioInputUnit_context::setupFomart()
//{
//	// Describe format
//	AudioStreamBasicDescription audioFormat;
//	audioFormat.mSampleRate			= 22050.00;
//	audioFormat.mFormatID			= kAudioFormatLinearPCM;
//	audioFormat.mFormatFlags		= kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
//	audioFormat.mFramesPerPacket	= 1;
//	audioFormat.mChannelsPerFrame	= 1;
//	audioFormat.mBitsPerChannel		= 16;
//	audioFormat.mBytesPerPacket		= 2;
//	audioFormat.mBytesPerFrame		= 2;
//	
//    OSStatus err = noErr;
//	// Apply format
//	err = AudioUnitSetProperty(_audioUnit, 
//								  kAudioUnitProperty_StreamFormat, 
//								  kAudioUnitScope_Input, 
//								  kInputBus, 
//								  &audioFormat, 
//								  sizeof(audioFormat));
//    return err;
//}

OSStatus AudioInputUnit_context::setupBuffers()
{
    _buffer = new BytesBuffer(2<<10);
//    _ring = std::auto_ptr<RingBufferA>(new RingBufferA(2 << 10, 2 << 2) );
    return noErr;
}



OSStatus AudioInputUnit_context::start()
{
    OSStatus status = noErr;
	status = AudioOutputUnitStart(_audioUnit);
	checkErr(status);
#ifdef RECODESTREAM
    //cout << "open file IosServerStream.pcm\n";
    _recodestreamfile = fopen("IosServerStream.pcm", "wb");
#endif    
    return  status;
}

/**
 Stop the audioUnit
 */
OSStatus AudioInputUnit_context::stop()
{
	OSStatus status = noErr;
    status = AudioOutputUnitStop(_audioUnit);
	checkErr(status);
#ifdef RECODESTREAM
    //cout << "close file IosServerStream.pcm\n";
    fclose(_recodestreamfile);
#endif
    return  status;
}


//////////////////////////////////////////////////////////////////
AudioInputUnit& AudioInputUnit::instance()
{
    static AudioInputUnit ref;
    return ref;
}

AudioInputUnit::AudioInputUnit()
{
    _ctx = std::auto_ptr< AudioInputUnit_context>(new AudioInputUnit_context);
}

void AudioInputUnit::uninitialize()
{
    _ctx->uninitialize();
}

bool AudioInputUnit::isInitialized()
{
    return _ctx->isInitialized();
}

void AudioInputUnit::initialize(float sampleRate, int channel, int sampleDeep)
{
    _ctx->initialize(sampleRate, channel, sampleDeep);
}

OSStatus AudioInputUnit::startRec()
{
    return _ctx->start();
}


OSStatus AudioInputUnit::stopRec()
{
    return _ctx->stop();
}

bool AudioInputUnit::isRunning()
{
    return _ctx->isRunning();
}

void AudioInputUnit::flush()
{
//    _ctx->_ring->flush();
}


//bool AudioInputUnit::getData(PopBufferChunkRef chunkref, size_t waitMicroSeconds )
//{
////    return _ctx->_ring->pop(chunkref, waitMicroSeconds);
//}

AudioInputUnit::~AudioInputUnit()
{
    
}




