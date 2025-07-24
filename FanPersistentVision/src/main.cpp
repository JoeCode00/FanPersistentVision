#include <Arduino.h>
#include <QNEthernet.h>
#include <bitset>
#include <cmath>
#include <chrono>

qindesign::network::EthernetUDP udp; // Create a UDP object

void setup()
{
  IPAddress staticIp(10, 0, 0, 3);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress gateway(10, 0, 0, 3);
  Serial.begin(9600);

  // Start Ethernet and obtain an IP address (using DHCP)
  if (qindesign::network::Ethernet.begin(staticIp, subnet, gateway) == 0)
  {
    Serial.println("Failed to configure Ethernet");
    while (true)
      ; // Halt if initialization fails
  }


  Serial.print("My IP address: ");
  Serial.println(qindesign::network::Ethernet.localIP());

  // Listen for UDP packets on a specific port
  udp.begin(8888); // Replace with your desired UDP port
}

void loop()
{
  // Check for incoming UDP packets
  int packetSize = udp.parsePacket();

  if (packetSize > 0)
  {
    auto start_time = std::chrono::high_resolution_clock::now();
    // Serial.print("Received UDP packet from ");
    // Serial.print(udp.remoteIP());
    // Serial.print(":");
    // Serial.print(udp.remotePort());
    // Serial.print(", size: ");
    // Serial.println(packetSize);

    for (int i = 0; i < floor(packetSize/3.0); i++)
    {
      std::bitset<8> G = static_cast<std::bitset<8>>(udp.read());
      std::bitset<8> R = static_cast<std::bitset<8>>(udp.read());
      std::bitset<8> B = static_cast<std::bitset<8>>(udp.read());
      
      std::bitset<24> bsGRB;
      for (size_t j = 0; j < 8; ++j) {
        bsGRB[j] = G[j];
        bsGRB[j + 8] = R[j];
        bsGRB[j + 16] = B[j];
      }
      
      //SPI code goes here

      // for (std::size_t j = 0; j < bsGRB.size(); j++)
      // {
      //   Serial.print(bsGRB[j]); // Print each bit
      // }
      // Serial.println();
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    float FPS = floor(1000*1000/duration_us.count());
    float bitsPerFrame = 8*3*2000;
    // Serial.println(String(FPS) + " Frames/second");
    // Serial.println(String(FPS*bitsPerFrame) + " b/second");
     Serial.println(String(FPS*bitsPerFrame/1000/1000) + " Mb/second");
    // Serial.println(String(packetSize));
    // while (udp.available()) {
    //   Serial.write(udp.read());
    // }
    
  }
// Call Ethernet.loop() regularly on non-Teensy platforms for stack processing
// On Teensy, this is handled automatically via EventResponder
// #if !defined(CORE_TEENSY)
//   Ethernet.loop();
// #endif
}
