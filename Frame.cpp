//
//  Frame.cpp
//  
#include "Frame.h"

namespace Frame 
{
  // which pin to send on
  byte sendPin;

  // current bits yet to be sent
  volatile byte data[10];
  
  volatile byte datum; 
  
  // how many bits
  volatile byte bitsToGo;
  
  // how many bytes
  volatile int currentByte; // which byte we are sending
  
  // the current bit
  byte currentBit;
   
  // have we sent the first of the two pulses?
  boolean sentFirst;

  boolean previousLevel;

  byte frame;
  bool isEbu;

/* create a hardware timer */
  hw_timer_t * timer = NULL;
   
  void begin (const byte pin, bool ebu)
  { 
    sendPin = pin;
    isEbu = ebu;
    
    digitalWrite (sendPin, HIGH);
    pinMode (sendPin, OUTPUT);
    
    bitsToGo = 0;    // force data load
    currentByte = 9; 
    frame = 24;
    
    data[8] = B11111100; // constant sync bytes
    data[9] = B10111111;
       
    // prescale from 80MHz
    // 80 / 80 * 250 = 250uS for EBU (25fps)
    // 7 / 80 * 2381 = 208.33uS for SMPTE (30 fps)
    timer = timerBegin(0, isEbu ? 80 : 7, true);    
    timerAttachInterrupt(timer, &onTimer, true); 
    timerAlarmWrite(timer, isEbu ? 250 : 2381, true);
  
    /* Start an alarm */
    timerAlarmEnable(timer);
  } 
  
  
  void IRAM_ATTR onTimer()
  {      
    // if we have no more bits to send, queue up the next byte 
    if (bitsToGo == 0)
    {
      if(currentByte == 9)
      {      
         currentByte = -1;
    
         if(frame == (isEbu ? 24 : 29))
         {
            frame = 0;
            loadFrameData(); 
         }
         else
         {
            frame++;
            updateFrameCount(frame); 
         }
      }  
      currentByte++;
      datum = data[currentByte];        

      bitsToGo = 8;
    } 
    
    // second pulse is the right way around
    if (sentFirst)
    {
      // if a 1 then invert level
      if (currentBit == 1)
      {
         previousLevel =  !previousLevel;
         digitalWrite (sendPin, previousLevel);     
      }
      
      // ready for next one
      sentFirst = false;
     
      bitsToGo--;  // one less bit to send
      return;
    }  
  
    // get low-order bit
    currentBit = datum & 1;
    // ready for next time
    datum = datum >> 1;
   
    // first halfbit is opposite
    previousLevel = !previousLevel;
    digitalWrite (sendPin, previousLevel);        
  
    // next time we sent other way around
    sentFirst = true;      
  } 

  void loadFrameData()
  {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
      return;
    }
  
    byte frames = 0;
    byte secs = timeinfo.tm_sec;
    byte mins = timeinfo.tm_min;
    byte hours = timeinfo.tm_hour;
    
    data[0] = frames %10;
    data[1] = frames / 10;
    data[2] = secs % 10;
    data[3] = secs / 10;
    data[4] = mins % 10;
    data[5] = mins / 10;
    data[6] = hours % 10;
    data[7] = hours / 10; 
  
    updateParity();
  }
  
  void updateFrameCount(byte frame)
  { 
    data[0] = frame %10;
    data[1] = frame / 10;
  
    updateParity();
  }
  
  void updateParity()
  { 
    byte ones = parity(data[0]) +
    parity(data[1]) +
    parity(data[2]) +
    parity(data[3]) +
    parity(data[4]) +
    parity(data[5]) +
    parity(data[6]) +
    parity(data[7]);
  
    if((ones & 1) == 0)
    {
       data[7] |= B0001000;
    }
  }
  
  byte parity(byte v)
  {
    v ^= v >> 4;
    v &= 0xf;
    
    return (0x6996 >> v) & 1;
  }

}

  
