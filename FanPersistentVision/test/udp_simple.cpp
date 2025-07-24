#include <Arduino.h>
#include <QNEthernet.h>
using namespace qindesign::network;

EthernetUDP udp; // Create a UDP object

void setup()
{
  IPAddress staticIp(10, 0, 0, 3);    // Your desired static IP address
  IPAddress subnet(255, 255, 255, 0); // Standard subnet mask for a /24 network
  IPAddress gateway(10, 0, 0, 3);     // Your router's IP address
  Serial.begin(9600);                 // Initialize serial communication for debugging

  // Start Ethernet and obtain an IP address (using DHCP)
  if (Ethernet.begin(staticIp, subnet, gateway) == 0)
  {
    Serial.println("Failed to configure Ethernet");
    while (true)
      ; // Halt if initialization fails
  }

  // You can also use a static IP address:
  // IPAddress ip(192, 168, 1, 100);
  // IPAddress gateway(192, 168, 1, 1);
  // IPAddress subnet(255, 255, 255, 0);
  // Ethernet.begin(ip, gateway, subnet);

  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  // Listen for UDP packets on a specific port
  udp.begin(8888); // Replace with your desired UDP port
}

void loop()
{
  // Check for incoming UDP packets
  delay(100);

  Serial.println(Ethernet.localIP());
  int packetSize = udp.parsePacket();

  // Check if a valid packet (including zero-length packets) was received
  if (packetSize >= 0)
  {
    Serial.print("Received UDP packet from ");
    Serial.print(udp.remoteIP());
    Serial.print(":");
    Serial.print(udp.remotePort());
    Serial.print(", size: ");
    Serial.println(packetSize);
    uint8_t buffer[packetSize];

    for (int i = 0; i < packetSize; i++)
    {
      int h = udp.read();
      Serial.println(String(h));
    }
    // // Read the packet data and print it
    // while (udp.available()) {
    //   Serial.println(udp.read(), HEX);
    // }
    // Serial.println(); // Add a newline for readability
  }
// Call Ethernet.loop() regularly on non-Teensy platforms for stack processing
// On Teensy, this is handled automatically via EventResponder
#if !defined(CORE_TEENSY)
  Ethernet.loop();
#endif
}
