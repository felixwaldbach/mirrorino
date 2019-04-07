/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/


// Includes
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// Constants and Objects
#define DHTPIN 13 // Digital Pin that the DHT sensor is connected to
#define DHTTYPE DHT22 // Type of the DHT sensor

DHT dht(DHTPIN, DHTTYPE); // DHT Sensor Object

// Update these with values suitable for your network.
const char* ssid = "SmartMirror"; // WiFi Network Name
const char* password = "smartmirror2019"; // WiFi Network Password
const char* mqtt_server = "192.168.178.200"; // MQTT Broker Address

WiFiClient espClient; // WiFi Client Object
PubSubClient client(espClient); // MQTT Client Object

char msg[50]; // Message variable used to send MQTT messages with a payload

/**
 * Function: setupWifi
 * Parameters: -
 * Function is called inside the setup function to connect the device to WiFi
 */
void setupWifi() {

  delay(10);
  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Print dots while the WiFi is connecting
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  // Print IP Address after connecting to WiFi
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * Function: callback
 * Parameters: 
 * *** topic: MQTT Topic of the incoming message
 * *** payload: Payload that has been sent together with the message
 * *** length: Length of the payload byte array
 * This function is called whenever an MQTT message reaches the MQTT Client on this device
 */
void callback(char* topic, byte* payload, unsigned int length) {
  // Print the incoming message
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("], Payload: [");

  // Convert payload to string
  char strPayload[50];
  int i;
  for (i = 0; i < length; i++) {
    strPayload[i] = (char)payload[i];
  }
  strPayload[i] = '\0';
  Serial.print(strPayload);
  Serial.println("]");

  // Check the Topic of the incoming message
  if(strcmp(topic, "outdoor/dht22/receive/values") == 0) { // Value request message
    Serial.println("DHT 22 Value Request received");
    readDht(); // Read current temperature and humidity of the DHT sensor
  }
}

/**
 * Function: reconnect
 * Parameters: -
 * This function is invoked when the MQTT client loses connection to the MQTT Broker
 * It attempts to reconnect every 5 seconds until a connection is established again
 */
void reconnect() {
  // Loop until connection is re-established
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-Indoor"; // Client ID for the Broker
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Resubscribe to the relevant topics for this client
      client.subscribe("outdoor/dht22/receive/values"); // Request value message is received periodically when the OutdoorWidget is activated on the Mirror
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/**
 * Function: readDht
 * Parameters: -
 * Function is invoked inside the dhtTimer and reads current temperature and humidity values of the DHT sensor
 */
void readDht() {
    Serial.print("Reading temperature and humidity\n");
    float h = dht.readHumidity(); // Humidity
    float t = dht.readTemperature(); // Temperature
    if(!isnan(t) && !isnan(h)) { // Check if values are valid
      snprintf (msg, 75, "%f", t);
      Serial.print("Publish temperature: ");
      Serial.println(msg);
      client.publish("outdoor/dht22/send/temperature", msg); // Send the temperature data to the mirror

      snprintf(msg, 75, "%f", h);
      Serial.print("Publish humidity: ");
      Serial.println(msg);
      client.publish("outdoor/dht22/send/humidity", msg); // Send the humidity data to the mirror
    }
    delay(1000);
}

/**
 * Function: setup
 * Parameters: -
 * The setup function is executed at start to set up the Pins, WiFi connection and MQTT Connection
 */
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  
  setupWifi(); // Connect to WiFi
  
  client.setServer(mqtt_server, 1883); // Connect to MQTT
  client.setCallback(callback);

  dht.begin(); // Start DHT sensor
}

/**
 * Function: loop
 * Parameters: -
 * Main loop
 */
void loop() {

  if (!client.connected()) { // If MQTT Client is disconnected, attempt to reconnect
    reconnect();
  }
  client.loop();
}
