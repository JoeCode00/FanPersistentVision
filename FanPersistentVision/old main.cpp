#include <Arduino.h>
#include <TeensyThreads.h>
#include <QNEthernet.h>
#include <bitset>
#include <cmath>
#include <chrono>

#include <constants.h>

using namespace qindesign::network;

IPAddress serverIP;

IPAddress staticIP;
IPAddress subnetMask;
IPAddress gateway;

EthernetClient client;

int connection_retries = 0;
int max_connection_retries = 5;
int connection_retry_delay_ms = 1000;

const int BYTES_PER_BLADE = BYTES_PER_LED * LEDS_PER_BLADE;
const int PACKET_SIZE = BLADES * BYTES_PER_BLADE;

volatile bool print = false;

volatile uint8_t preamble[PREAMBLE_LENGTH];
volatile uint8_t frame = preamble[PREAMBLE_FRAME0_INDEX];

long long bits_in = 0;
float kilobits_per_second = 0;
uint8_t old_frame = 0;
int frame_count = 0;
float frames_per_second = 0.0;

volatile uint8_t bytesOut[BLADES][BYTES_PER_BLADE];
const long DELAY_US = 1;

long long current_time_ns()
{
  auto current = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(current.time_since_epoch()).count();
}

double current_time_s()
{
  return current_time_ns() / 1000000000.0;
}

double start_s = current_time_s();

double elapsed_time_s()
{
  return current_time_s() - start_s;
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

// void udp_client()
// {

//   int packetSize = udp.parsePacket();

//   if (packetSize > 0)
//   {
//     Serial.println("Frame");
// uint8_t blank_preamble[PREAMBLE_LENGTH];
// udp.readBytes(blank_preamble, PREAMBLE_LENGTH);
// for (int i = 0; i < PREAMBLE_LENGTH; i++) // Rewrite into memcpy?
// {
//   preamble[i] = blank_preamble[i];
// }
// frame = preamble[PREAMBLE_FRAME_INDEX];

// if (frame > old_frame || (old_frame >> 245 && frame < 10))
// {
//   // Serial.println(frame);
//   for (int blade = 0; blade < BLADES; blade++)
//   {
//     uint8_t bladeBuffer[BYTES_PER_BLADE];
//     udp.readBytes(bladeBuffer, BYTES_PER_BLADE);
//     // memcpy(bytesOut[blade], bladeBuffer, BYTES_PER_BLADE);
//     for (int i = 0; i < BYTES_PER_BLADE; i++)
//     {
//       bytesOut[blade][i] = bladeBuffer[i];
//     }
//   }
//   frame_count++;
//   Serial.println(int(frame_count/elapsed_time_s()));
// }
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
//   }
// }

void connect()
{
  Serial.println("Connecting to server...");
  if (client.connect(serverIP, SERVER_PORT))
  {
    Serial.println("Connected to server!");
  }
  else
  {
    Serial.print("Connection failed - error: ");
    Serial.println(client.status()); // Get the connection status code
  }
}

// uint8_t buffer[PREAMBLE_LENGTH + LEDS_CRICUMF * LEDS_PER_BLADE * BYTES_PER_LED];
uint8_t preamble_buff[PREAMBLE_LENGTH];
uint8_t blade0[LEDS_CRICUMF * LEDS_PER_BLADE * BYTES_PER_LED / 5];
uint8_t blade1[LEDS_CRICUMF * LEDS_PER_BLADE * BYTES_PER_LED / 5];
uint8_t blade2[LEDS_CRICUMF * LEDS_PER_BLADE * BYTES_PER_LED / 5];
uint8_t blade3[LEDS_CRICUMF * LEDS_PER_BLADE * BYTES_PER_LED / 5];
uint8_t blade4[LEDS_CRICUMF * LEDS_PER_BLADE * BYTES_PER_LED / 5];
void read()
{
  while (client.available())
  {
    
    
    bits_in = bits_in + PREAMBLE_LENGTH + LEDS_CRICUMF * LEDS_PER_BLADE * BYTES_PER_LED * 8;
    kilobits_per_second = bits_in / elapsed_time_s() / 100000;
    
    frame_count++;
    frames_per_second = frame_count / elapsed_time_s();
    Serial.println(String("Frame ") + uint8_to_3_str(preamble_buff[PREAMBLE_FRAME0_INDEX]) + String(": ") + String(frames_per_second) +String(" FPS, ")+ String(kilobits_per_second) + String(" Kb/s")); // Kb/s
    // code
  }
}

void check_connection()
{
  if (!client.connected() && (connection_retries < max_connection_retries))
  {
    Serial.println("Server disconnected or was never connected.");
    client.stop();
    delay(connection_retry_delay_ms);
    connect();
    connection_retries++;
    if (connection_retries >= max_connection_retries)
    {
      Serial.println("Max connection retries reached, permanantly disconnecting.");
    }
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

  serverIP.fromString(SERVER_IP);
  staticIP.fromString(TEENSY_IP);
  subnetMask.fromString(SUBNET_MASK);
  gateway.fromString(GATEWAY);

  Ethernet.begin(staticIP, subnetMask, gateway);
  Ethernet.waitForLink(5000);

  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  connect();
}

void loop()
{
  read();
  check_connection();
  delay(10);
}