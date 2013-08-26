#include "AudioIOunit.h"

#import <AudioToolbox/AudioToolbox.h>
//#include <CoreAudio/CoreAudio.h>

//#include "CAStreamBasicDescription.h"
#define kOutputBus 0
#define kInputBus 1



extern void CheckError(OSStatus error, const char *operation);




//#define RECODESTREAM
class AudioIOunit_context
{
public:
    friend class AudioIOunit;
    AudioIOunit_context();
    ~AudioIOunit_context();

    

    bool isRunning();
    
    
    
//  output    
    void init(float sampleRate, int channel, int sampleDeep);
    friend OSStatus palybackRenderProc(void *inRefCon,
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList * ioData);
    
    void CreateMyAUGraph();
    bool playbackIsRunning();
    void startPlayback();
    void stopPlayback();
    void setupOutputFormat(float sampleRate, int channels, int sampleBitDeeps);
    static void palybackRopBufferCallback(ChunkInfoRef ref, void* userData, PopbufState state, unsigned fillneedsize);
    void palybackpopBufferCallback(ChunkInfoRef ref, PopbufState state, unsigned fillneedsize);
    
//   mic input
    void initMicphoneUnit();
    
//   mixer 
    void initMixerUnit();
private:
 
    AudioUnit                   _inputUnit;
    std::auto_ptr<RingBufferA>  _recRing;
	AudioBufferList             _inputBuffer;   // this will hold the latest data from the microphone       
    std::auto_ptr<RingBufferA>      _recodeRing;
//    

//      
    AudioUnit                       _fileUnit;
    
//    
    AudioUnit                       _mixerUnit;
    
//    output
    AUGraph                         _graph;
    AudioUnit                       _outputUnit;       
    std::auto_ptr<RingBufferA>      _playbackRing;
    AudioStreamBasicDescription     _outputAudioFormat;
    PopBufferChunk                  _packbackPopchunk;
    AudioBufferList                 * _curOutputABL;
    unsigned                        _fillOffset;
    unsigned char*                  _tempBuf;    
    
//   mixer

#ifdef RECODESTREAM
    FILE* _recodestreamfile;
#endif    
};




void AudioIOunit_context::palybackRopBufferCallback(ChunkInfoRef ref, void* userData, PopbufState state, unsigned fillneedsize)
{
    AudioIOunit_context* This = (AudioIOunit_context*)userData;
    This->palybackpopBufferCallback(ref, state, fillneedsize);
}

//#define TESTPOPDATA
void AudioIOunit_context::palybackpopBufferCallback(ChunkInfoRef ref, PopbufState state, unsigned fillneedsize)
{
#ifdef TESTPOPDATA    
    static unsigned totalcount = 0;
    static IceUtil::Time _recievedTimeLastTime;
    IceUtil::Time receivedTime = IceUtil::Time().now();
    if (_recievedTimeLastTime == IceUtil::Time() && _recievedTimeLastTime != receivedTime) 
    {
        _recievedTimeLastTime = receivedTime;
    }
    else
    {               
        IceUtil::Time costTime = receivedTime - _recievedTimeLastTime;
        if (costTime.toSeconds() > 2) {
            totalcount = 0;
        }
        _recievedTimeLastTime = receivedTime;
    }
    
#endif
    if (e_whole == state) 
    {
        assert(ref->m_dataSize == fillneedsize);
        memcpy(_curOutputABL->mBuffers[0].mData,  ref->m_data,  fillneedsize);
        _curOutputABL->mBuffers[0].mDataByteSize = fillneedsize;
        ///////////////////////////////////////////////////////////////////////
#ifdef TESTPOPDATA      
        
        for (unsigned i=0; i <ref->m_dataSize; ++i) 
        {            
            if (totalcount++%16 == 0) 
                printf("\n%0x0h:", totalcount/16);
            displayHexBin( ref->m_data[i]);
            printf(" ");
        }        
#endif
        ///////////////////////////////////////////////////////////////////////
        return ;
    }
    
    if (_tempBuf == NULL) {
        _tempBuf = (unsigned char*)malloc(fillneedsize);
    }
    
    
    
    if (e_start == state) {
        _fillOffset = 0;
    }
    
    memcpy(_tempBuf+_fillOffset, ref->m_data, ref->m_dataSize);
    _fillOffset += ref->m_dataSize;
    
    if (e_end == state) 
    {
        assert(_fillOffset == fillneedsize);
        memcpy(_curOutputABL->mBuffers[0].mData, _tempBuf, fillneedsize );
        _curOutputABL->mBuffers[0].mDataByteSize = fillneedsize;
        //        ///////////////////////////////////////////////////////////////////////
#ifdef TESTPOPDATA       
        //static unsigned totalcount = 0;
        for (unsigned i=0; i < fillneedsize; ++i) 
        {            
            if (totalcount++%16 == 0) 
                printf("\n%0x0h:", totalcount/16);
            displayHexBin( _tempBuf[i]);
            printf(" ");
        }        
#endif
        //        ///////////////////////////////////////////////////////////////////////        
    }
    
}



OSStatus palybackRenderProc(void *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp *inTimeStamp,
                         UInt32 inBusNumber,
                         UInt32 inNumberFrames,
                         AudioBufferList * ioData)
{
    // Notes: ioData contains buffers (may be more than one!)
    // Fill them up as much as you can. Remember to set the size value in each buffer to match how
    // much data is in the buffer.
    AudioIOunit_context* This = (AudioIOunit_context*)inRefCon;
    
	This->_curOutputABL = ioData;
    assert(ioData->mNumberBuffers == 1);
    
    
    This->_packbackPopchunk.m_callback = AudioIOunit_context::palybackRopBufferCallback;
    This->_packbackPopchunk.m_userData = This;
    This->_packbackPopchunk.m_fillDataSize = This->_outputAudioFormat.mBytesPerFrame*inNumberFrames;
    
    
    This->_playbackRing->pop(&(This->_packbackPopchunk ) );
    
    return noErr;    
}

/////////////////



AudioIOunit_context::AudioIOunit_context()
{
    //initAudioInput();
}


AudioIOunit_context::~AudioIOunit_context()
{
    if (_tempBuf != NULL) {
        free(_tempBuf);
    }
    AUGraphUninitialize (_graph);
    AUGraphClose(_graph);
    //AudioUnitUninitialize(_audioUnit);
}


void AudioIOunit_context::setupOutputFormat(float sampleRate, int channels, int sampleBitDeepth)
{
	//AudioStreamBasicDescription _outputAudioFormat;
	_outputAudioFormat.mSampleRate			= sampleRate;
	_outputAudioFormat.mFormatID			= kAudioFormatLinearPCM;
	_outputAudioFormat.mFormatFlags		= kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	_outputAudioFormat.mFramesPerPacket	= 1;
	_outputAudioFormat.mChannelsPerFrame	= channels;
	_outputAudioFormat.mBitsPerChannel		= sampleBitDeepth;
	_outputAudioFormat.mBytesPerPacket		= sampleBitDeepth/8;
	_outputAudioFormat.mBytesPerFrame		= sampleBitDeepth/8;     
}

void AudioIOunit_context::initMixerUnit()
{
    AudioComponentDescription mixercd = {0}; 
    mixercd.componentType = kAudioUnitType_Mixer;
    mixercd.componentSubType = kAudioUnitSubType_MultiChannelMixer; 
    mixercd.componentManufacturer = kAudioUnitManufacturer_Apple;   


    // Get component
	AudioComponent inputComponent = AudioComponentFindNext(NULL, &mixercd);
	
	// Get audio units
	CheckError(AudioComponentInstanceNew(inputComponent, &_mixerUnit) , "AudioComponentInstanceNew mixer unit failed" );

}


void AudioIOunit_context::CreateMyAUGraph()
{
    // Create a new AUGraph
    CheckError(NewAUGraph(&_graph),
               "NewAUGraph failed");
    
    // Generate a description that matches default output
    AudioComponentDescription outputcd = {0};
    outputcd.componentType = kAudioUnitType_Output;
    outputcd.componentSubType = kAudioUnitSubType_RemoteIO;        //????????
    outputcd.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponent comp = AudioComponentFindNext(NULL, &outputcd);
    if (comp == NULL) {
        printf ("Can't get output unit"); exit (-1);
    }
    
    // Adds a node with above description to the graph
    AUNode outputNode;
    CheckError(AUGraphAddNode(_graph,
                              &outputcd,
                              &outputNode),
               "AUGraphAddNode[kAudioUnitSubType_DefaultOutput] failed");
#ifdef PART_II
    // Insert Listings 8.24 - 8.27 here
#else
    // Opening the graph opens all contained audio units
    // but does not allocate any resources yet
    CheckError(AUGraphOpen(_graph),
               "AUGraphOpen failed");
    
    // Get the reference to the AudioUnit object for the
    // output graph node
    CheckError(AUGraphNodeInfo(_graph,
                               outputNode,
                               NULL,
                              &_outputUnit),
               "AUGraphNodeInfo failed");
    
    // Set the stream format on the output unit's input scope
    UInt32 propertySize = sizeof (AudioStreamBasicDescription);
    CheckError(AudioUnitSetProperty(_outputUnit,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Input,
                                    kOutputBus,
                                    &_outputAudioFormat,
                                    propertySize),
               "Couldn't set stream format on output unit");
    
    
    //setup callback proc
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = palybackRenderProc;
    callbackStruct.inputProcRefCon = this;
    
    CheckError(AudioUnitSetProperty(_outputUnit,
                                    kAudioUnitProperty_SetRenderCallback,
                                    kAudioUnitScope_Global,
                                    0,
                                    &callbackStruct,
                                    sizeof(callbackStruct)),
               "Couldn't set render callback on output unit");
    // Now initialize the graph (causes resources to be allocated)
    CheckError(AUGraphInitialize(_graph),
               "AUGraphInitialize failed");
    
    _playbackRing = std::auto_ptr<RingBufferA>(new RingBufferA(2 << 10, 2 << 2) );
#endif
}


bool AudioIOunit_context::playbackIsRunning()
{
    Boolean running;
    CheckError(AUGraphIsRunning(_graph, &running) , "check AUGraphIsRunning failed");
    printf("running %s\n", ((running == true) ? "true": "false") );
    return running;
}

void AudioIOunit_context::startPlayback()
{ 
    CheckError(AUGraphStart(_graph), "AUGraphStart failed");
#ifdef RECODESTREAM
    //cout << "open file IosServerStream.pcm\n";
    _recodestreamfile = fopen("IosServerStream.pcm", "wb");
#endif    
}

/**
 Stop the audioUnit
 */
void AudioIOunit_context::stopPlayback()
{
    CheckError(AUGraphStop(_graph), "AUGraphStop faild");
#ifdef RECODESTREAM
    fclose(_recodestreamfile);
#endif
}










////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AudioIOunit& AudioIOunit::instance()
{
    static AudioIOunit ref;
    return ref;
}

AudioIOunit::AudioIOunit()
{
    _ctx = std::auto_ptr<AudioIOunit_context>(new AudioIOunit_context);
}

void AudioIOunit::init(float sampleRate, int channel, int sampleBitDeep)
{
    _ctx->setupOutputFormat(sampleRate, channel, sampleBitDeep);
}

void AudioIOunit::startRec()
{
   // return _ctx->start();
}

void AudioIOunit::stopRec()
{

}

//bool AudioIOunit::popRecodeData(PopBufferChunkRef chunkref, size_t waitMicroSeconds )
//{
//    //return _ctx->_ring->pop(chunkref, waitMicroSeconds);
//}



/////////////////
//void AudioIOunit::setOutputFormat(float sampleRate, int channels, int sampleBitDeepth)
//{
//    _ctx->setupOutputFormat(sampleRate, channels, sampleBitDeepth);
//}


bool AudioIOunit::playbackIsRunning()
{
    return _ctx->playbackIsRunning();
}

void AudioIOunit::startPalyback()
{
    _ctx->startPlayback();
}

void AudioIOunit::stopPlayback()
{
    _ctx->stopPlayback();
}

void AudioIOunit::initOutput(float sampleRate, int channels, int sampleBitDeepth)
{
    _ctx->setupOutputFormat(sampleRate, channels, sampleBitDeepth);
    _ctx->CreateMyAUGraph();
}

unsigned char* AudioIOunit::getHungryPlayBuf(unsigned getHungryPlayBuf)
{
    return _ctx->_playbackRing->get(getHungryPlayBuf);
}

void AudioIOunit::putDataToPlayBuf(unsigned datasize)
{
    _ctx->_playbackRing->put(datasize);
}

void AudioIOunit::flushPlaybackBuffers()
{
    _ctx->_playbackRing->flush();
}




AudioIOunit::~AudioIOunit()
{
    
}




