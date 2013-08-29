Scope       Bus           Semantics                               Property
Access
Input       1       Input from hardware to I/O unit                 Read-only
Output      1      Output from I/O unit to program or other units     Read/write
Input       0       Input to I/O unit from program or other units       Read/write
Output      0      Output from I/O unit to hardware               Read-only

The difference is that the scope is input to and output from the unit,
whereas the bus represents input to and output from
the hardware/program or other units (but only for I/O units)	


so
record render_callback  bus is 1
play   render_callback  bus is 0


record, 
output to program or other units(bus 0)    kAudioUnitProperty_StreamFormat            kAudioUnitProperty_ShouldAllocateBuffer
Input from hardware to I/O unit(bus 1)      kAudioOutputUnitProperty_EnableIO         kAudioUnitProperty_SetRenderCallback


play 
Input to I/O unit from program or other units (bus 1)    kAudioUnitProperty_StreamFormat          kAudioUnitProperty_ShouldAllocateBuffer
Output from I/O unit to hardware(bus  0)                kAudioOutputUnitProperty_EnableIO         kAudioUnitProperty_SetRenderCallback

define  inputbus as HAL when record  1
define outputbus as HAL when play    0
inputbus also represent program/other units when play
outputbus also represetn program/other units when recordss


outputscope  outputbus     mean  output to HAL
inputscope   inputbus      mean  input from HAL

outputscope inputbus  mean   output to other
inputscope outputbus  mean   input from other