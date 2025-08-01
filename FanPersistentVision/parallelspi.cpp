#include <FlexIO_t4.h>
#include <FlexIOSPI.h>
#include <TeensyThreads.h>
#include <queue>
// #include <mutex>

// Define pins
const int MOSI_PINS[5] = {7, 14, 17, 20, 26};
const int SCK_PINS[5] = {3, 15, 18, 21, 27};
const int MISO_PINS[5] = {1, 1, 1, 1, 1};
const int CS_PINS[5] = {2, 2, 2, 2, 2};
const int CLOCK_FREQ = 2500000; // 2.5 MHz

// Create FlexIO SPI instances for each channel
FlexIOSPI spi0(MOSI_PINS[0], SCK_PINS[0], MISO_PINS[0], CS_PINS[0]);
FlexIOSPI spi1(MOSI_PINS[1], SCK_PINS[1], MISO_PINS[1], CS_PINS[1]);
FlexIOSPI spi2(MOSI_PINS[2], SCK_PINS[2], MISO_PINS[2], CS_PINS[2]);
FlexIOSPI spi3(MOSI_PINS[3], SCK_PINS[3], MISO_PINS[3], CS_PINS[3]);
FlexIOSPI spi4(MOSI_PINS[4], SCK_PINS[4], MISO_PINS[4], CS_PINS[4]);

FlexIOSPI *spiChannels[5] = {&spi0, &spi1, &spi2, &spi3, &spi4};

// Data structure for transmission
struct TransmitData
{
    uint8_t *data;
    size_t length;
};

// Queues and mutexes for thread-safe data handling
std::queue<TransmitData> dataQueues[5];
Threads::Mutex queueMutexes[5];
volatile bool channelBusy[5] = {false, false, false, false, false};

// Thread functions for each SPI channel
void spiThread(void *arg)
{
    int channelIndex = *(int *)arg;
    FlexIOSPI *spi = spiChannels[channelIndex];

    // Configure pins for this channel
    // spi->begin();
    // spi->setClockSpeed(CLOCK_FREQ);
    spi->beginTransaction(FlexIOSPISettings(CLOCK_FREQ, MSBFIRST, SPI_MODE0));

    while (1)
    {
        TransmitData tx;
        bool hasData = false;

        // Check for new data to transmit
        queueMutexes[channelIndex].lock();
        if (!dataQueues[channelIndex].empty())
        {
            tx = dataQueues[channelIndex].front();
            dataQueues[channelIndex].pop();
            hasData = true;
        }
        queueMutexes[channelIndex].unlock();

        if (hasData)
        {
            channelBusy[channelIndex] = true;
            uint32_t data = static_cast<uint32_t>(*tx.data);
            spi->transferNBits(data, sizeof(data));
            delete[] tx.data; // Clean up allocated memory
            channelBusy[channelIndex] = false;
        }
        Serial.print(channelIndex);
        Serial.print(" yields");
        threads.yield();
    }
}

// Function to queue data for transmission on a specific channel
bool queueTransmission(int channel, uint8_t *data, size_t length)
{
    if (channel < 0 || channel >= 5)
        return false;

    // Create copy of data
    uint8_t *dataCopy = new uint8_t[length];
    memcpy(dataCopy, data, length);

    TransmitData tx = {dataCopy, length};

    queueMutexes[channel].lock();
    dataQueues[channel].push(tx);
    queueMutexes[channel].unlock();

    return true;
}

// Function to check if a channel is currently transmitting
bool isChannelBusy(int channel)
{
    if (channel < 0 || channel >= 5)
        return true;
    return channelBusy[channel];
}

// Function to transmit data on all available channels
void transmitOnAllChannels(uint8_t *baseData, size_t length)
{
    for (int i = 0; i < 5; i++)
    {
        if (!isChannelBusy(i))
        {
            uint8_t *channelData = new uint8_t[length];
            memcpy(channelData, baseData, length);
            // Modify data for each channel to make it unique
            for (size_t j = 0; j < length; j++)
            {
                channelData[j] += i; // Add channel number to make data unique
            }
            queueTransmission(i, channelData, length);
            // delete[] channelData; // REMOVE THIS LINE!
        }
    }
}

void setup()
{
    // Initialize serial for debugging
    Serial.begin(115200);
    // while (!Serial) delay(10);

    Serial.println("Initializing parallel SPI channels...");

    // Create threads for each SPI channel
    static int channelIndices[5] = {0, 1, 2, 3, 4};
    for (int i = 0; i < 5; i++)
    {
        threads.addThread(spiThread, &channelIndices[i]);
    }
}

void loop()
{
    Serial.println("1");
    threads.delay(1000);
    // static unsigned long lastTransmitTime = 0;
    // const unsigned long TRANSMIT_INTERVAL = 100; // Transmit every 100ms

    // // Periodic transmission example
    // if (millis() - lastTransmitTime >= TRANSMIT_INTERVAL)
    // {
    //     lastTransmitTime = millis();
    //     Serial.println("2");
    //     // Example data packet
    //     uint8_t baseData[] = {0xAA, 0xBB, 0xCC, 0xDD};
    //     transmitOnAllChannels(baseData, sizeof(baseData));
    //     Serial.println("3");
    // }

    // // Optional: Monitor channel status (can be disabled if not needed)
    // static unsigned long lastStatusTime = 0;
    // const unsigned long STATUS_INTERVAL = 5000; // Status every 5 seconds
    // Serial.println("4");
    // if (millis() - lastStatusTime >= STATUS_INTERVAL)
    // {
    //     lastStatusTime = millis();
    //     Serial.println("\nChannel Status:");
    //     for (int i = 0; i < 5; i++)
    //     {
    //         Serial.printf("Channel %d: %s\n", i, isChannelBusy(i) ? "Busy" : "Ready");
    //     }
    // }
    // Serial.println("5");
    // // Allow other tasks to run
    // // yield();
}