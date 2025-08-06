#include <Arduino.h>
#include <chrono>
#include <FlexIO_t4.h>
#include <FlexIOSPI.h>
#include <QNEthernet.h>
#include <constants.h>

using namespace qindesign::network;

// Enable high-performance mode
#define QNETHERNET_BUFFERS_IN_RAM 1
#define QNETHERNET_ENABLE_RAW_FRAME_LOOPBACK 0

// TCP Window and Buffer Optimization
#define TCP_WND (32 * 1024)            // 32KB receive window (increase throughput)
#define TCP_SND_BUF (32 * 1024)        // 32KB send buffer
#define TCP_SND_QUEUELEN 16            // More segments in send queue
#define TCP_SNDLOWAT (TCP_SND_BUF / 2) // Send when buffer is half full

// TCP Segment Size Optimization
#define TCP_MSS 1460                 // Maximum segment size (Ethernet MTU - headers)
#define TCP_CALCULATE_EFF_SEND_MSS 1 // Calculate effective MSS

// TCP Connection Limits
#define MEMP_NUM_TCP_PCB 8        // Max concurrent TCP connections
#define MEMP_NUM_TCP_PCB_LISTEN 4 // Max listening sockets

// TCP Timing Optimization
#define TCP_TMR_INTERVAL 10                      // TCP timer interval (ms) - faster retransmit
#define TCP_FAST_INTERVAL TCP_TMR_INTERVAL       // Fast retransmit timer
#define TCP_SLOW_INTERVAL (2 * TCP_TMR_INTERVAL) // Slow timer

// Nagle Algorithm Control
#define TCP_NODELAY 1 // Disable Nagle for low latency
#define LWIP_TCP_NODELAY 1

// TCP Keepalive
#define LWIP_TCP_KEEPALIVE 1
#define TCP_KEEPIDLE_DEFAULT 7200000 // 2 hours in ms
#define TCP_KEEPINTVL_DEFAULT 75000  // 75 seconds between probes
#define TCP_KEEPCNT_DEFAULT 9        // 9 probes before timeout

// Memory Pool Optimization
#define PBUF_POOL_SIZE 32      // More packet buffers
#define PBUF_POOL_BUFSIZE 1524 // Buffer size (MTU + headers)
#define MEM_SIZE (64 * 1024)   // 64KB heap for lwIP

// ARP Table Optimization
#define ARP_TABLE_SIZE 10 // ARP cache entries
#define ARP_MAXAGE 300    // ARP entry timeout (5 minutes)

// IP Fragment Handling
#define IP_FRAG 1             // Enable IP fragmentation
#define IP_REASS_MAX_PBUFS 20 // Max pbufs for reassembly

// Statistics and Debugging (disable for production)
#define LWIP_STATS 0 // Disable stats for performance
#define LWIP_DEBUG 0 // Disable debug output

// Checksum Offloading (if supported by hardware)
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_CHECK_IP 1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1

IPAddress serverIP;

IPAddress staticIP;
IPAddress subnetMask;
IPAddress gateway;

EthernetClient client;

int connection_retries = 0;
int max_connection_retries = 2;
int connection_retry_delay_ms = 1000;

volatile bool frame_read = false;

int packet_length = PREAMBLE_LENGTH + LEDS_CRICUMF * LEDS_PER_BLADE * BYTES_PER_LED;
int frame = 0;
int frame_buffer_in_start = 0;

long long bits_in = 0;
float megabits_per_second = 0;
uint8_t old_frame = 0;
int frame_count = 0;
float frames_per_second = 0.0;

bool ws2812b_spi_data[LEDS_PER_BLADE * BYTES_PER_LED * 8 * 3]; // *3 due to the 3 bits needed for PWM emulation

const int MOSI_PINS[BLADES] = {2, 8, 34, 38, 19}; // Known FlexIO pins
const int MISO_PINS[BLADES] = {3, 9, 35, 39, 18}; // Use -1 if not needed
const int SCK_PINS[BLADES] = {4, 10, 36, 40, 14};  // Known FlexIO pins
const int CS_PINS[BLADES] = {-1, -1, -1, -1, -1}; // Use -1 for no CS
const int CLOCK_FREQ = 1000000;                   // 1 MHz - reduced for testing

FlexIOSPI spi0(MOSI_PINS[0], SCK_PINS[0], MISO_PINS[0], CS_PINS[0]);
FlexIOSPI spi1(MOSI_PINS[1], SCK_PINS[1], MISO_PINS[1], CS_PINS[1]);
FlexIOSPI spi2(MOSI_PINS[2], SCK_PINS[2], MISO_PINS[2], CS_PINS[2]);
FlexIOSPI spi3(MOSI_PINS[3], SCK_PINS[3], MISO_PINS[3], CS_PINS[3]);
FlexIOSPI spi4(MOSI_PINS[4], SCK_PINS[4], MISO_PINS[4], CS_PINS[4]);
FlexIOSPI spi_channels[5] = {spi0, spi1, spi2, spi3, spi4}; // Use same instance for all channels for testing
// FlexIOSPI spi_channels[BLADES];

long long current_time_ns()
{
  auto current = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(current.time_since_epoch()).count();
}

double current_time_s()
{
  return current_time_ns() / 1000000000.0;
}

double elapsed_time_s(double start_s)
{
  return current_time_s() - start_s;
}

void print(auto printable, bool new_line = true)
{
  if (new_line)
  {
    Serial.println(printable);
  }
  else
  {
    Serial.print(printable);
  }
}

String uint8_to_3_str(uint8_t num, bool left_space = true)
{
  String ls = String("");
  if (left_space)
  {
    ls = String(" ");
  }
  if (num < 10)
  {
    return ls + String("  ") + String(num);
  }
  if (num < 100)
  {
    return ls + String(" ") + String(num);
  }
  return ls + String(num);
}

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

void parse_preamble(uint8_t *buffer)
{
  frame = buffer[PREAMBLE_FRAME_INDEX0] * 255 + buffer[PREAMBLE_FRAME_INDEX1];
  frame_buffer_in_start = buffer[PREAMBLE_FRAME_BUFFER_IN_INDEX0] * 255 + buffer[PREAMBLE_FRAME_BUFFER_IN_INDEX1];
}

bool read(uint8_t *buffer)
{
  bool frame_read = false;
  if (client.available())
  {
    static int bytes_received = 0;
    static bool reading_frame = false;

    // Start reading a new frame
    if (!reading_frame)
    {
      bytes_received = 0;
      reading_frame = true;
    }

    // Read available data directly into buffer
    int available = client.available();
    int bytes_to_read = min(available, packet_length - bytes_received);

    if (bytes_to_read > 0)
    {
      int actual_read = client.read(&buffer[bytes_received], bytes_to_read);
      if (actual_read > 0)
      {
        bytes_received += actual_read;

        // Check if we have a complete frame
        if (bytes_received >= packet_length)
        {
          // Verify magic number in preamble
          if (buffer[PREAMBLE_MAGIC_NUMBER_INDEX] == PREAMBLE_MAGIC_NUMBER)
          {
            parse_preamble(buffer);
            frame_read = true;
          }
          reading_frame = false;
          bytes_received = 0;
        }
      }
    }
  }
  return frame_read;
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

uint8_t *allocate(int bytes)
{
  uint8_t *memory = (uint8_t *)malloc(bytes);

  if (memory == nullptr)
  {
    Serial.println(String("ERROR: Failed to allocate memory of") + String(bytes) + String(" bytes!"));
  }
  return memory;
}

void move_uint8_t(uint8_t *source, int source_start_index, int transfer_length_B, uint8_t *destination, int destination_start_index = 0)
{
  memcpy(&destination[destination_start_index], &source[source_start_index], transfer_length_B);
}

// // Define the pins for MOSI and SCK
// const int MOSI_PIN = 7; // Example pin, choose any available digital output pin
// const int SCK_PIN = 13; // Example pin, choose any available digital output pin

// // Define the delay for a 2.5 MHz clock
// // A 2.5 MHz clock means a period of 1 / 2.5 MHz = 0.4 microseconds = 400 nanoseconds.
// // Each clock cycle involves setting SCK HIGH and then LOW, so each phase is 200 nanoseconds.
// const int BIT_DELAY_NS = 180; // Delay for each phase of the clock (half a cycle)
// const int NUM_LEDS = 2;

// void waitClock()
// {
//   // digitalWriteFast(MOSI_PIN, LOW);
//   digitalWriteFast(SCK_PIN, HIGH); // Raise SCK
//   delayNanoseconds(BIT_DELAY_NS);  // Wait for half a clock cycle

//   digitalWriteFast(SCK_PIN, LOW); // Lower SCK
//   delayNanoseconds(BIT_DELAY_NS); // Wait for the other half of the clock cycle
// }

// void sendPWMBits(uint8_t pwmBit, int blade)
// {
//   digitalWriteFast(MOSI_PINS[blade], 1);
//   waitClock();
//   digitalWriteFast(MOSI_PINS[blade], pwmBit);
//   waitClock();
//   digitalWriteFast(MOSI_PINS[blade], 0);
// }

// void sendByte(byte data, int blade)
// {
//   for (int i = 7; i >= 0; i--)
//   { // MSB first
//     uint8_t pwmBit = (data >> i) & 0x01;
//     sendPWMBits(pwmBit, blade);
//   }
// }

// void ws2812b_spi_out(uint8_t *frame_buffer, int circ_offset, int blade)
// {
//   for (int LED = 0; LED < LEDS_PER_BLADE; LED++)
//   {
//     int LED_offset = circ_offset + LED * 3;
//     for (int color_index = 0; color_index < BYTES_PER_LED; color_index++)
//     {
//       int color_offset = LED_offset + color_index;
//       uint8_t color_value = *(frame_buffer + color_offset);

//       for (int i = 7; i >= 0; i--)
//       { // MSB first
//         uint8_t pwmBit = (color_value >> i) & 0x01;
        // sendPWMBits(pwmBit, blade);
//       }
//     }
//     delayMicroseconds(55);
//   }
// }

void setup()
{
  Serial.begin(115200);
  while (!Serial && millis() < 3000)
    ; // Wait for serial connection

  for (int i = 0; i < BLADES; i++)
  {
    if (!spi_channels[i].begin())
    {
      Serial.println(String("Could not begin SPI channel ")+String(i));
      while (1){};
    }
  }
  Serial.println("All SPI channels begun");
  // test_spi
  for (int i = 0; i < BLADES; i++)
  {
    spi_channels[i].beginTransaction(FlexIOSPISettings(CLOCK_FREQ, MSBFIRST, SPI_MODE0));
  }
  Serial.println("All SPI channels transacting");

  uint8_t *frame_buffer = allocate(packet_length);
  for (int i = 0; i < packet_length; i++)
  {
    frame_buffer[i] = 0;
  }

  serverIP.fromString(SERVER_IP);
  staticIP.fromString(TEENSY_IP);
  subnetMask.fromString(SUBNET_MASK);
  gateway.fromString(GATEWAY);

  Ethernet.begin(staticIP, subnetMask, gateway);
  Ethernet.waitForLink(5000);

  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  // connect();

  // for (int i = 0; i < BLADES; i++)
  // {
  //   pinMode(MOSI_PINS[i], OUTPUT);
  //   pinMode(SCK_PINS[i], OUTPUT);
  // }

  // digitalWriteFast(SCK_PIN, LOW); // Ensure SCK starts low

  double start_s = current_time_s();
  // int rotation_index = 0; // Out of circ pixels

  // sendByte(0, 0);
  // sendByte(0, 0);
  // sendByte(0, 0);
  // delayMicroseconds(55);

  // for (int i = 0; i < BLADES; i++)
  // {
  //   spi_channels[i] = FlexIOSPI(MOSI_PINS[i], SCK_PINS[i], MISO_PINS[i], CS_PINS[i]);
  // }

  // for (int i = 0; i < BLADES; i++)
  // {
  //   spi_channels[i].beginTransaction(FlexIOSPISettings(CLOCK_FREQ, MSBFIRST, SPI_MODE0));
  // }

  // while (1)
  // {
  //   frame_read = read(frame_buffer);
  //   if (frame_read)
  //   {
  //     bits_in += packet_length * 8;
  //     megabits_per_second = bits_in / elapsed_time_s(start_s) / 1000000;

  //     frame_count++;
  //     frames_per_second = frame_count / elapsed_time_s(start_s);

  //     bool diagnostic_prints = false;
  //     if (diagnostic_prints)
  //     {
  //       print(String("Frame ") + String(frame_count) + String(": "), false);
  //       print(String(frames_per_second) + String(" FPS, "), false);
  //       print(String(megabits_per_second) + String(" Mb/s, "), false);
  //       print(String(megabits_per_second / 8 / 3 * 1000000) + String(" Pixels/s,"), false);
  //       for (int i = 0; i < PREAMBLE_LENGTH; i++)
  //       {
  //         print(uint8_to_3_str(frame_buffer[i]), false);
  //       }
  //       print("");
  //     }
  //     // for (int blade = 0; blade < BLADES; blade++)
  //     // {
  //     //   int blade_rotation = rotation_index + blade * int(LEDS_CRICUMF / BLADES);
  //     //   int circ_offset = PREAMBLE_LENGTH + blade_rotation * LEDS_PER_BLADE * BYTES_PER_LED;
  //     //   ws2812b_spi_out(frame_buffer, circ_offset, blade);
  //     // }
  //     print("First row grb: ", false);
  //     for (int rotation_index = 0; rotation_index < LEDS_CRICUMF; rotation_index++)
  //     {
  //       for (int blade = 0; blade < BLADES; blade++)
  //       {
  //         int blade_rotation = rotation_index + blade * int(LEDS_CRICUMF / BLADES);
  //         int circ_offset = PREAMBLE_LENGTH + blade_rotation * LEDS_PER_BLADE * BYTES_PER_LED;
  //         uint8_t *circ_ptr = frame_buffer + circ_offset;

  //         //FLEXIO SPI
  //         // spi_channels[blade].transferBufferNBits(circ_ptr, nullptr, LEDS_PER_BLADE * BYTES_PER_LED, 8);

  //         // for (int radial_LED = 0; radial_LED < LEDS_PER_BLADE; radial_LED++)
  //         // {
  //         //   uint8_t *led_ptr = circ_ptr + radial_LED * BYTES_PER_LED;
  //         //   for (int color_index = 0; color_index < BYTES_PER_LED; color_index++)
  //         //   {
  //         //     int offset = radial_LED * BYTES_PER_LED + color_index;
  //         //     uint8_t *color_ptr = circ_ptr + offset;
  //         //     // Wrap offset if needed
  //         //     if ((color_ptr - frame_buffer) >= packet_length)
  //         //     {
  //         //       color_ptr = frame_buffer + PREAMBLE_LENGTH + ((color_ptr - frame_buffer) - packet_length);
  //         //     }
  //         //     // uint8_t color_byte = *color_ptr;
  //         //     sendByte(*color_ptr, blade);
  //         //     if (radial_LED == 0)
  //         //     {
  //         //       print(uint8_to_3_str(*color_ptr), false);
  //         //     }
  //         //   }
  //         // }
  //         // delayMicroseconds(55);
  //         print("|", false);
  //       }
  //       print("");
  //     }
  //   }
  //   check_connection();
  //   // delay(1);
  // }

  Serial.println("");
  // Clean up memory
  free(frame_buffer);

  Serial.println("Memory freed successfully");
}

void loop()
{
  // Nothing to do in loop
  delay(10);
}