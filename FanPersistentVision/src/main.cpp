#include <Arduino.h>
#include <SPI.h>
#include <TeensyThreads.h>

#define SS_PIN 10 // Example Slave Select pin
SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0); // Example SPI settings
