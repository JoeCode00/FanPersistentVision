#include <Arduino.h>

// Define the pins for MOSI and SCK
const int MOSI_PIN = 7; // Example pin, choose any available digital output pin
const int SCK_PIN = 13; // Example pin, choose any available digital output pin

// Define the delay for a 2.5 MHz clock
// A 2.5 MHz clock means a period of 1 / 2.5 MHz = 0.4 microseconds = 400 nanoseconds.
// Each clock cycle involves setting SCK HIGH and then LOW, so each phase is 200 nanoseconds.
const int BIT_DELAY_NS = 200; // Delay for each phase of the clock (half a cycle)
const int NUM_LEDS = 2;


void waitClock(){
  // digitalWriteFast(MOSI_PIN, LOW);
  digitalWriteFast(SCK_PIN, HIGH); // Raise SCK
  delayNanoseconds(BIT_DELAY_NS);  // Wait for half a clock cycle

  digitalWriteFast(SCK_PIN, LOW); // Lower SCK
  delayNanoseconds(BIT_DELAY_NS); // Wait for the other half of the clock cycle
}

void sendPWMBits(uint8_t pwmBit){
  if (pwmBit == 0){
    digitalWriteFast(MOSI_PIN, 1);
    waitClock();
    digitalWriteFast(MOSI_PIN, 0);
    waitClock();
    digitalWriteFast(MOSI_PIN, 0);
  }
  else if (pwmBit == 1){
    digitalWriteFast(MOSI_PIN, 1);
    waitClock();
    digitalWriteFast(MOSI_PIN, 1);
    waitClock();
    digitalWriteFast(MOSI_PIN, 0);
  }
  else {
    digitalWriteFast(MOSI_PIN, 0);
    waitClock();
    digitalWriteFast(MOSI_PIN, 0);
    waitClock();
    digitalWriteFast(MOSI_PIN, 0);
  }
}


void sendByte(byte data)
{
  for (int i = 7; i >= 0; i--)
  { // MSB first
    uint8_t pwmBit = (data >> i) & 0x01;
    sendPWMBits(pwmBit);
  }
}

void sendColor(int g, int r, int b){
  sendByte(static_cast<uint8_t>(g));
  sendByte(static_cast<uint8_t>(r));
  sendByte(static_cast<uint8_t>(b));
  delayMicroseconds(55); // Short delay between transmissions for demonstration
}

void setup()
{
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(SCK_PIN, OUTPUT);
  digitalWriteFast(SCK_PIN, LOW); // Ensure SCK starts low
  for (int i = 0; i < NUM_LEDS; i++){
    sendColor(0, 0, 0);
  }
}

void loop()
{
  sendColor(255, 0, 0);
  sendColor(255, 0, 0);
  delay(2000);
  sendColor(0, 0, 0);
  sendColor(0, 0, 0);
  delay(2000);
  // for (int r = 0; r < 255; r++){
  //   // for (int g = 0; g < 255; g++){
  //   //   for (int b = 0; b < 255; b++){
  //   //     for (int i = 0; i < NUM_LEDS; i++){
  //   //       sendColor(g, r, b);
  //   //       sendColor(0,0,0);
          
  //   //     }
  //   //     delay(10);
  //   //   }
  //   // }
  //   sendColor(0, r, 0);
  //   sendColor(0, 0, 0);
  //   delay(100);
  // }
  
}
