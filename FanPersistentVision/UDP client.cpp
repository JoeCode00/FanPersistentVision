#include <Arduino.h>
#include <QNEthernet.h>
#include <bitset>
#include <cmath>
#include <chrono>

qindesign::network::EthernetUDP udp; // Create a UDP object
// float frames_processed = 0;
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
auto start_time = std::chrono::high_resolution_clock::now();

void loop()
{

  // Check for incoming UDP packets
  int packetSize = udp.parsePacket();

  if (packetSize > 0)
  {
    for (int i = 0; i < floor(packetSize / 3.0); i++)
    {
      std::bitset<8> G = static_cast<std::bitset<8>>(udp.read());
      std::bitset<8> R = static_cast<std::bitset<8>>(udp.read());
      std::bitset<8> B = static_cast<std::bitset<8>>(udp.read());

      std::bitset<24> bsGRB;
      for (size_t j = 0; j < 8; ++j)
      {
        bsGRB[j] = G[j];
        bsGRB[j + 8] = R[j];
        bsGRB[j + 16] = B[j];
      }

      // SPI code goes here
    }
    // frames_processed++;

    // Serial.println(String(packetSize));
    // while (udp.available()) {
    //   Serial.write(udp.read());
    // }
  }

  // auto now = std::chrono::high_resolution_clock::now();
  // auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
  // float bitsPerFrame = 8*3*4000;
  // Serial.println(String((frames_processed/1000*bitsPerFrame/1000)/(duration_s.count()))+ " Mb/s");
  // Serial.println(String((frames_processed*4500.0)/(duration_s.count()))+ " Pixels/s");
}
