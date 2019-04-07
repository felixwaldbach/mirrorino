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
#include <SimpleTimer.h>

// Constants and Objects
#define DHTPIN 13 // Digital Pin that the DHT sensor is connected to
#define DHTTYPE DHT22 // Type of the DHT sensor

#define PIRPIN 4 // Digital Pin that the PIR sensor is connected to
#define NO_MOTION_MAX 5 // Number of Loops that the PIR sensor does not check for a motion after the 3 minute timeout expired

DHT dht(DHTPIN, DHTTYPE); // DHT Sensor Object

// the timer object
SimpleTimer dhtTimer; // Timer Object for the DHT sensor 
SimpleTimer pirTimer; // Timer Object for the PIR sensor
int pirTimerId; // Timer ID of the pirTimer, needed for toggling the timer

// Update these with values suitable for your network.
const char* ssid = "SmartMirror"; // WiFi Network Name
const char* password = "smartmirror2019"; // WiFi Network Password
const char* mqtt_server = "192.168.178.200"; // MQTT Broker Address

WiFiClient espClient; // WiFi Client Object
PubSubClient client(espClient); // MQTT Client Object

char msg[50]; // Message variable used to send MQTT messages with a payload

bool timeout = false; // is set to true when a face is recognized on the mirror to disable motion detection
bool processing = false; // is set to true when the mirror is performing face recognition and set to false afterwards
int noMotionCtr = 0; // Counter variable to check the number of loops after the 3 minutes timeout

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
 * Function: togglePirTimer
 * Parameters: -
 * Enables or disables the pirTimer depending on its current state
 */
void togglePirTimer() {
  pirTimer.toggle(pirTimerId);
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
  if(strcmp(topic, "indoor/pir/receive/timeout") == 0) { // Timeout message
    processing = false;
    if(strcmp(strPayload, "true") == 0) {
      Serial.println("Timeout message [true] received");
      timeout = true;
      togglePirTimer(); // Disable the pirTimer to stop checking for motion
      pirTimer.setTimeout(180000, togglePirTimer); // Enable the pirTimer again after 3 Minutes
      delay(1000);
    } else {
      Serial.println("Timeout message [false] received");
    }
  } else if(strcmp(topic, "indoor/dht22/receive/values") == 0) { // Value Request message
    Serial.println("DHT 22 Value Request received");
    dhtTimer.setTimeout(500, readDht); // Start the DHT Timer to read current Temperature and Humidity 
  }
}

/**
 * Function: reconnect
 * Parameters: -
 * This function is invoked when the MQTT client loses connection to the MQTT Broker
 * It attempts to reconnect every 5 seconds until a connection is established again
 */
void reconnect() {
  processing = false;
  // Loop until connection is re-established
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-Indoor"; // Client ID for the Broker
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Resubscribe to the relevant topics for this client
      client.subscribe("indoor/pir/receive/timeout"); // Timeout is received whenever face recognition is performed on the mirror; Payload is either true to set the timeout or false to keep checking motion
      client.subscribe("indoor/dht22/receive/values"); // Request value message is received periodically when the IndoorWidget is activated on the Mirror
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
    togglePirTimer(); // Disable the PIR while reading sensor data
    Serial.print("Reading temperature and humidity\n");
    float h = dht.readHumidity(); // Humidity
    float t = dht.readTemperature(); // Temperature
    if(!isnan(t) && !isnan(h)) { // Check if values are valid
      snprintf (msg, 75, "%f", t);
      Serial.print("Publish temperature: ");
      Serial.println(msg);
      client.publish("indoor/dht22/send/temperature", msg); // Send the temperature data to the mirror

      snprintf(msg, 75, "%f", h);
      Serial.print("Publish humidity: ");
      Serial.println(msg);
      client.publish("indoor/dht22/send/humidity", msg); // Send the humidity data to the mirror
    }
    delay(6000); // Wait 6 seconds before re-enabling the pirTimer to prevent it from automatically detecting motion because of the DHT timer
    togglePirTimer();
}


/*
 * Function: readPir
 * Parameters: -
 * Function is invoked inside the pirTimer and reads motion value of the PIR sensor
 */
void readPir() {
    if(!processing) { // If Face Recognition is not already performed, check if motion is detected
      long state = digitalRead(PIRPIN);
      if(state == HIGH) {
        Serial.println("Motion detected!");
        snprintf (msg, 75, "1");
        client.publish("indoor/pir/send/motion", msg); // If motion detected, send message to the broker
        processing = true; // Processing is set to true because the Mirror performs Face Recognition now
      } else { // Face recognition has been performed
        if(timeout) { // 3 minute timeout just expired
          Serial.println("Timeout activated");
          if(noMotionCtr == NO_MOTION_MAX) { // Send message to broker after 5 seconds to destroy the session
            Serial.println("Destroying session!");
            snprintf (msg, 75, "0");
            client.publish("indoor/pir/send/motion", msg); // Send motion data to broker to destroy session
            noMotionCtr = 0; // Reset counter
            timeout = false; // Timeout is over
          } else {
            noMotionCtr++; // Increase counter
          }
        }
      }
    } else {
      delay(2000);
    }
    delay(1000);
}

/**
 * Function: setup
 * Parameters: -
 * The setup function is executed at start to set up the Pins, WiFi connection and MQTT Connection and timeout to calibrate the Pir sensor
 */
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(PIRPIN, INPUT);   // declare sensor as input
  Serial.begin(115200);
  
  setupWifi(); // Connect to WiFi
  
  client.setServer(mqtt_server, 1883); // Connect to MQTT
  client.setCallback(callback);

  dht.begin(); // Start DHT sensor
  
  Serial.println("Calibrating...");
  delay(60000); // Delay so that the PIR can calibrate itself
  Serial.println("Motion detection ready");
  pirTimerId = pirTimer.setInterval(1000, readPir); // Activate Pir Timer
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

  pirTimer.run(); // Start Pir Timer
  dhtTimer.run(); // Start DHT Timer
}
