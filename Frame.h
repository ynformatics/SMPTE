//
//  Frame.h
//  

#ifndef _Frame_h
#define _Frame_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

namespace Frame 
{
  // which pin to send on
  extern byte sendPin;
  
  // current bits yet to be sent
  //volatile byte data[];
  
  // how many bits
  extern volatile byte bitsToGo;
  
  // the current bit
  extern byte currentBit;
  
 // the previous bit
  extern boolean previousLevel;
  
  // have we sent the first of the two pulses?
  extern boolean sentFirst;
  
  void begin (const byte pin, bool ebu);
  void write (const byte data);

  void loadFrameData();
  void updateFrameCount(byte frame);
  void updateParity();
  byte parity(byte v);
  
  void IRAM_ATTR onTimer(); 
} 

#endif
