#include <Arduino.h>
#include <QNEthernet.h>
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

void read()
{
  while (client.available())
  {
    
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