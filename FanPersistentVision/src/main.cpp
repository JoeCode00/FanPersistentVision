#include <Arduino.h>
#include <TeensyThreads.h>
#include <QNEthernet.h>
#include <bitset>
#include <cmath>
#include <chrono>

const int LEDS_PER_BLADE = 1;
const int BYTES_PER_LED = 3;
const int BLADES = 5;

const int BYTES_PER_BLADE = BYTES_PER_LED * LEDS_PER_BLADE;
const int PACKET_SIZE = BLADES * BYTES_PER_BLADE;

volatile bool print = false;


volatile uint8_t bytesOut[BLADES][BYTES_PER_BLADE];
// uint8_t* str_adr[BYTES_PER_BLADE] = {
//     &bytesOut[0],
//     &bytesOut[1],
//     &bytesOut[2],
//     &bytesOut[3],
//     &bytesOut[4],
// };
const long DELAY_US = 1;

qindesign::network::EthernetUDP udp;
auto start_time = std::chrono::high_resolution_clock::now();

void async_print(int threadIndex)
{
  while (1)
  {
    if (print)
    {
      Serial.println(String(threadIndex)
      + String(": ")
      + String(bytesOut[threadIndex][0]) + String(" ")
      + String(bytesOut[threadIndex][1]) + String(" ")
      + String(bytesOut[threadIndex][2])

      );
      // Serial.println(String(threadIndex));
      threads.delay(10);
    }
  }
}

// void async_tester()
// {
//   while (1)
//   {
//     threads.delay_us(DELAY_US);
//     for (int j = 0; j < BLADES; j++)
//     {
//       bytesOut[j] = String(j);
//     }

//     threads.delay_us(DELAY_US);
//     for (int j = 0; j < BLADES; j++)
//     {
//       bytesOut[j] = "";
//     }
//   }
// }

void udp_client()
{
  int packetSize = udp.parsePacket();

  if (packetSize > 0)
  {
    for (int blade = 0; blade < BLADES; blade++)
    {
      uint8_t bladeBuffer[BYTES_PER_BLADE];
      udp.readBytes(bladeBuffer, BYTES_PER_BLADE);
      // memcpy(bytesOut[blade], bladeBuffer, BYTES_PER_BLADE);
      for (int i = 0; i < BYTES_PER_BLADE; i++)
      {
        bytesOut[blade][i] = bladeBuffer[i];
      }
    }
    print = true;
    threads.delay(1000);
    print = false;

  // std::bitset<24> bsGRB;
  // std::bitset<8> G;
  // std::bitset<8> R;
  // std::bitset<8> B;
  // for (int i = 0; i < floor(packetSize / 3.0); i++)
  // {
  //   G = static_cast<std::bitset<8>>(udp.read());
  //   R = static_cast<std::bitset<8>>(udp.read());
  //   B = static_cast<std::bitset<8>>(udp.read());

  //   for (size_t j = 0; j < 8; ++j)
  //   {
  //     bsGRB[j] = G[j];
  //     bsGRB[j + 8] = R[j];
  //     bsGRB[j + 16] = B[j];
  //   }

  //   // SPI code goes here
  // }
  // Serial.println("Frame");
  // frames_processed++;

  // Serial.println(String(packetSize));
  // while (udp.available()) {
  //   Serial.write(udp.read());
  //
  //
  // }
  // Serial.println(G.to_ulong());
  // Serial.println(bsGRB.to_string().c_str());
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Threads");
  // threads.addThread(async_tester);
  for (int i = 0; i < BLADES; i++)
  {
    threads.addThread(async_print, i);
  }

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
  // Serial.println("Loop");
  udp_client();
  print = true;
}