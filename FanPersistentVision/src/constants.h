#pragma once
#define LEDS_PER_BLADE 400
#define BYTES_PER_LED 3
#define BLADES 5

#define TEENSY_IP "10.0.0.3"
#define TEENSY_PORT 8888

#define SERVER_IP "10.0.0.2"
#define SERVER_PORT 12345

#define PREAMBLE_LENGTH 8
#define PREAMBLE_FRAME_INDEX 0
// pio run && /usr/bin/python3 /home/joseph/Documents/GitHub/FanPersistentVision/FanPersistentVision/UDPServer.py