#include "AudioPlayUnit.h"

#import <AudioToolbox/AudioToolbox.h>
#include <IceUtil/Time.h>
#define kOutputBus 0
#define kInputBus 1

#define checkErr( err) \
if(err) {\
OSStatus error = static_cast<OSStatus>(err);\
fprintf(stdout, " Error: %ld ->  %s:  %d\n",  error,\
__FILE__, \
__LINE__\
);\
fflush(stdout);\
return err; \
}         


extern void CheckError(OSStatus error, const char *operation);
extern void checkStatus(int status);


template <class T>
void displayHexBin(const T& v, bool hex = true)
{
    const unsigned char c2h[] = "0123456789ABCDEF";
    const unsigned char c2b[] = "01";
    
    unsigned char* p = (unsigned char*)&v;
    char* buf = new char [sizeof(T)*2+1];
    char* ptmp = buf;
    if(hex)
    {
        p = p + sizeof(T)-1;
        for (size_t i = 0; i < sizeof(T); i++, --p)
        {
            *buf++ = c2h[*p >> 4];
            *buf++ = c2h[*p & 0x0F];
        }
        *buf = '\0';
        //printf("hex format displayed as %s\n", ptmp);
        printf("%s", ptmp);
        
        delete [] ptmp;
        
    }
    else
    {
        p = (unsigned char*)&v;
        p = p + sizeof(T)-1;
        ptmp = buf = new char [sizeof(T)*8+1];
        for (size_t i = 0; i < sizeof(T); i++, --p)
        {
            for (int j = 0; j < 8; j++)
                *buf++ = c2b[(*p >> (7-j)) & 0x1];
        }
        *buf = '\0';
        //printf("bin format displayed as %s\n", ptmp);
        printf("%s", ptmp);
        delete [] ptmp;
    }
}



class AudioPlayUnit_context
{
public:
    friend class AudioPlayUnit;
    AudioPlayUnit_context();
    ~AudioPlayUnit_context();
    
    void initialize(float sampleRate, int channel, int sampleDeep);
    void uninitialize();
    bool isInitialized();
    OSStatus start();
    OSStatus stop();
    bool isRunning();
   
    
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
//    static void popBufferCallback(ChunkInfoRef ref, void* userData, PopbufState state, unsigned fillneedsize);
//    void popBufferCallback(ChunkInfoRef ref, PopbufState state, unsigned fillneedsize);
private:
    OSStatus enableIO();
    OSStatus callbackSetup();
    OSStatus setupFomart();
    OSStatus setupBuffers();
    inline void makeBufferSilent (AudioBufferList * ioData);
private:
//    std::auto_ptr<RingBufferA> _ring;
	AudioComponentInstance _audioUnit;
	//AudioBufferList _inputBuffer;   // this will hold the latest data from the microphone    
    AudioBufferList * _curOutputABL;
    unsigned          _fillOffset;
    unsigned char*    _tempBuf;
//    PopBufferChunk    _popchunk;
    unsigned          _totalreceivedSampleSize;
    bool              _isInitialized;
    
    AudioComponentInstance _audioUnit1;
};


AudioPlayUnit_context::AudioPlayUnit_context()
{
    _isInitialized = false;
    setupBuffers();
    _fillOffset = 0;
    _tempBuf = NULL;
}


AudioPlayUnit_context::~AudioPlayUnit_context()
{
    uninitialize();
    if (_tempBuf) {
        free(_tempBuf);
        _tempBuf = NULL;
    }
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


void AudioPlayUnit_context::initialize(float sampleRate, int channel, int sampleDeep) {
    if(_isInitialized)
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
	
	// Disable IO for recording
	UInt32 flag = 0;
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioOutputUnitProperty_EnableIO, 
								  kAudioUnitScope_Input, 
								  kInputBus,
								  &flag, 
								  sizeof(flag));
	checkStatus(status);
	
    
    
    
    flag = 1;
	// Enable IO for playback
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioOutputUnitProperty_EnableIO, 
								  kAudioUnitScope_Output, 
								  kOutputBus,
								  &flag, 
								  sizeof(flag));
	checkStatus(status);
	
	// Describe format
	AudioStreamBasicDescription audioFormat;

	audioFormat.mSampleRate			= sampleRate;
	audioFormat.mFormatID			= kAudioFormatLinearPCM;
	audioFormat.mFormatFlags		= kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	
	audioFormat.mChannelsPerFrame	= channel;
	audioFormat.mBitsPerChannel		= sampleDeep;    
    audioFormat.mFramesPerPacket	= 1;
	audioFormat.mBytesPerPacket		= audioFormat.mFramesPerPacket*sampleDeep/8;
	audioFormat.mBytesPerFrame		= channel*audioFormat.mFramesPerPacket*sampleDeep/8;
    audioFormat.mReserved = 0;

    
    
	// Apply format
//	status = AudioUnitSetProperty(audioUnit, 
//								  kAudioUnitProperty_StreamFormat, 
//								  kAudioUnitScope_Output, 
//								  kInputBus, 
//								  &audioFormat, 
//								  sizeof(audioFormat));
//	checkStatus(status);
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioUnitProperty_StreamFormat, 
								  kAudioUnitScope_Input, 
								  kOutputBus, 
								  &audioFormat, 
								  sizeof(audioFormat));
	checkStatus(status);
	
	
	// Set output callback
	AURenderCallbackStruct callbackStruct;
	
	
	callbackStruct.inputProc = playbackCallback;
	callbackStruct.inputProcRefCon = this;
	status = AudioUnitSetProperty(audioUnit, 
								  kAudioUnitProperty_SetRenderCallback, 
								  kAudioUnitScope_Global, 
								  kOutputBus,
								  &callbackStruct, 
								  sizeof(callbackStruct));
	checkStatus(status);
	


	
	// Initialise
	status = AudioUnitInitialize(audioUnit);
	checkStatus(status);
    _isInitialized = true;    
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
    
	This->_curOutputABL = ioData;
    assert(ioData->mNumberBuffers == 1);
    

//    This->_popchunk.m_callback = AudioPlayUnit_context::popBufferCallback;
//    This->_popchunk.m_userData = This;
//    This->_popchunk.m_fillDataSize = inNumberFrames*2;
//    
//    
//    This->_ring->pop(&(This->_popchunk ), 0.1*1000000 );      	
    return noErr;
}





//
//void AudioPlayUnit_context::popBufferCallback(ChunkInfoRef ref, void* userData, PopbufState state, unsigned fillneedsize)
//{
//    AudioPlayUnit_context* This = (AudioPlayUnit_context*)userData;
//    This->popBufferCallback(ref, state, fillneedsize);
//}

//#define TESTPOPDATA
//void AudioPlayUnit_context::popBufferCallback(ChunkInfoRef ref, PopbufState state, unsigned fillneedsize)
//{
//#ifdef TESTPOPDATA    
//    static unsigned totalcount = 0;
//    static IceUtil::Time _recievedTimeLastTime;
//    IceUtil::Time receivedTime = IceUtil::Time().now();
//    if (_recievedTimeLastTime == IceUtil::Time() && _recievedTimeLastTime != receivedTime) 
//    {
//        _recievedTimeLastTime = receivedTime;
//    }
//    else
//    {               
//        IceUtil::Time costTime = receivedTime - _recievedTimeLastTime;
//        if (costTime.toSeconds() > 2) {
//            totalcount = 0;
//        }
//        _recievedTimeLastTime = receivedTime;
//    }
//    
//#endif
//    if (e_whole == state) 
//    {
//        assert(ref->m_dataSize == fillneedsize);
//        memcpy(_curOutputABL->mBuffers[0].mData,  ref->m_data,  fillneedsize);
//        _curOutputABL->mBuffers[0].mDataByteSize = fillneedsize;
//        ///////////////////////////////////////////////////////////////////////
//#ifdef TESTPOPDATA      
//        
//        for (unsigned i=0; i <ref->m_dataSize; ++i) 
//        {            
//            if (totalcount++%16 == 0) 
//                printf("\n%0x0h:", totalcount/16);
//            displayHexBin( ref->m_data[i]);
//            printf(" ");
//        }        
//#endif
//        ///////////////////////////////////////////////////////////////////////
//        return ;
//    }
//    
//    if (_tempBuf == NULL) {
//       _tempBuf = (unsigned char*)malloc(fillneedsize);
//    }
//    
//
//    
//    if (e_start == state) {
//        _fillOffset = 0;
//    }
//    
//    memcpy(_tempBuf+_fillOffset, ref->m_data, ref->m_dataSize);
//    _fillOffset += ref->m_dataSize;
//    
//    if (e_end == state) 
//    {
//        assert(_fillOffset == fillneedsize);
//        memcpy(_curOutputABL->mBuffers[0].mData, _tempBuf, fillneedsize );
//        _curOutputABL->mBuffers[0].mDataByteSize = fillneedsize;
////        ///////////////////////////////////////////////////////////////////////
//#ifdef TESTPOPDATA       
//        static unsigned totalcount = 0;
//        for (unsigned i=0; i < fillneedsize; ++i) 
//        {            
//            if (totalcount++%16 == 0) 
//                printf("\n%0x0h:", totalcount/16);
//            displayHexBin( _tempBuf[i]);
//            printf(" ");
//        }        
//#endif
////        ///////////////////////////////////////////////////////////////////////        
//    }
//    
//}



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
	
    checkErr(err);
    return auhalRunning;
}




OSStatus AudioPlayUnit_context::start()
{   
    OSStatus status = noErr;
	status = AudioOutputUnitStart(_audioUnit);
	checkErr(status);

    return  status;
}

/**
 Stop the audioUnit
 */
OSStatus AudioPlayUnit_context::stop()
{
    
	OSStatus status = noErr;
    status = AudioOutputUnitStop(_audioUnit);
	CheckError(status, "AudioOutputUnitStop");
    
    return  status;
}


OSStatus AudioPlayUnit_context::setupBuffers()
{
//    _ring = std::auto_ptr<RingBufferA>(new RingBufferA(2 << 11, 2 << 2) );
    return noErr;
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


OSStatus AudioPlayUnit::startPlay()
{
    return _ctx->start();
}


OSStatus AudioPlayUnit::stopPlay()
{
    return _ctx->stop();
}

bool AudioPlayUnit::isRunning()
{
    return _ctx->isRunning();
}

void AudioPlayUnit::flush()
{
//    _ctx->_ring->flush();
}


unsigned char* AudioPlayUnit::getBuffer(unsigned & limitSize, unsigned waitTimeMicroSeconds)
{
//    return _ctx->_ring->get(limitSize, waitTimeMicroSeconds);
}

void AudioPlayUnit::fillBuffer(unsigned bufferSize)
{
//    _ctx->_ring->put(bufferSize);
}

AudioPlayUnit::~AudioPlayUnit()
{
    
}




