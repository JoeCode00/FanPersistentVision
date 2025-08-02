#include <Arduino.h>
#include <TeensyThreads.h>
#include <QNEthernet.h>
#include <bitset>
#include <cmath>
#include <chrono>

#include <constants.h>

const int BYTES_PER_BLADE = BYTES_PER_LED * LEDS_PER_BLADE;
const int PACKET_SIZE = BLADES * BYTES_PER_BLADE;

volatile bool print = false;

volatile uint8_t preamble[PREAMBLE_LENGTH];
volatile uint8_t frame = preamble[PREAMBLE_FRAME_INDEX];

uint8_t old_frame = 0;
float frame_count = 0.0;
float frames_per_second = 0.0;

volatile uint8_t bytesOut[BLADES][BYTES_PER_BLADE];
const long DELAY_US = 1;

qindesign::network::EthernetUDP udp;



long long current_time_ns(){
  auto current = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(current.time_since_epoch()).count();
}

double current_time_s(){
  return current_time_ns()/1000000000.0;
}

double start_s = current_time_s();

double elapsed_time_s(){
  return current_time_s()-start_s;
}

String uint8_to_3_str(uint8_t num)
{
  if (num < 10)
  {
    return String("  ") + String(num);
  }
  if (num < 100)
  {
    return String(" ") + String(num);
  }
  return String(num);
}

void async_print(int threadIndex)
{
  while (1)
  {
    if (threadIndex == 0 && print)
    {
      Serial.print(uint8_to_3_str(frame) + String(": "));
      int bytes_to_print = 3;
      for (int byte = 0; byte < bytes_to_print; byte++)
      {
        String printable = uint8_to_3_str(bytesOut[threadIndex][byte]);
        Serial.print(printable);
        Serial.print(" ");
      }
      Serial.println("");

      // Serial.println(String(threadIndex));
      threads.delay(10);
    }
  }
}

void udp_client()
{
  
  int packetSize = udp.parsePacket();

  if (packetSize > 0)
  {
    uint8_t blank_preamble[PREAMBLE_LENGTH];
    udp.readBytes(blank_preamble, PREAMBLE_LENGTH);
    for (int i = 0; i < PREAMBLE_LENGTH; i++) // Rewrite into memcpy?
    {
      preamble[i] = blank_preamble[i];
    }
    frame = preamble[PREAMBLE_FRAME_INDEX];

    if (frame > old_frame || (old_frame >> 245 && frame < 10))
    {
      // Serial.println(frame);
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
      frame_count++;
      Serial.println(int(frame_count/elapsed_time_s()));
    }
    // print = true;
    // threads.delay(1000);
    // print = false;

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

  String teensy_ip = String(TEENSY_IP);
  int teensy_ip_array[4];
  int partCount = 0;

  // Loop to extract parts until no more delimiters are found
  while (teensy_ip.length() > 0)
  {
    int delimiterIndex = teensy_ip.indexOf('.');

    if (delimiterIndex == -1)
    {                                                   // No more delimiters found
      teensy_ip_array[partCount++] = teensy_ip.toInt(); // Add the remaining part
      break;
    }
    else
    {
      teensy_ip_array[partCount++] = teensy_ip.substring(0, delimiterIndex).toInt();
      teensy_ip = teensy_ip.substring(delimiterIndex + 1); // Remove processed part
    }
  }

  Serial.println(teensy_ip_array[0]);
  Serial.println(teensy_ip_array[1]);
  Serial.println(teensy_ip_array[2]);
  Serial.println(teensy_ip_array[3]);

  IPAddress staticIp(teensy_ip_array[0], teensy_ip_array[1], teensy_ip_array[2], teensy_ip_array[3]);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress gateway(teensy_ip_array[0], teensy_ip_array[1], teensy_ip_array[2], teensy_ip_array[3]);

  // Start Ethernet and obtain an IP address (using DHCP)
  if (qindesign::network::Ethernet.begin(staticIp, subnet, gateway) == 0)
  {
    Serial.println("Failed to configure Ethernet");
    while (true)
      ; // Halt if initialization fails
  }

  Serial.print("My IP address: ");
  Serial.println(qindesign::network::Ethernet.localIP());

  int teensyPort = int(TEENSY_PORT);
  // Listen for UDP packets on a specific port
  udp.begin(teensyPort); // Replace with your desired UDP port
}

void loop()
{
  // Serial.println("Loop");
  udp_client();
}