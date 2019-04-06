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

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#include <SimpleTimer.h>

#define DHTPIN 13
#define DHTTYPE DHT22

#define PIRPIN 4
#define NO_MOTION_MAX 5

DHT dht(DHTPIN, DHTTYPE);

// the timer object
SimpleTimer dhtTimer;
SimpleTimer pirTimer;

// Update these with values suitable for your network.

const char* ssid = "SmartMirror";
const char* password = "smartmirror2019";
const char* mqtt_server = "192.168.2.104";

WiFiClient espClient;
PubSubClient client(espClient);

char msg[50];

bool timeout = false;
bool processing = false;
int noMotionCtr = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("], Payload: [");

  char strPayload[50];
  int i;
  for (i = 0; i < length; i++) {
    strPayload[i] = (char)payload[i];
  }
  strPayload[i] = '\0';
  Serial.print(strPayload);
  Serial.println("]");
  
  if(strcmp(topic, "indoor/pir/receive/timeout") == 0) {
    processing = false;
    if(strcmp(strPayload, "true") == 0) {
      Serial.println("Timeout message [true] received");
      timeout = true;
      delay(180000); // 3 minutes timeout 
    } else {
      Serial.println("Timeout message [false] received");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-Indoor";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("indoor/pir/receive/timeout");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// a function to be executed periodically
void readDht() {
    Serial.print("Reading temperature\n");
    float t = dht.readTemperature();
    if(!isnan(t)) {
      snprintf (msg, 75, "%f", t);
      Serial.print("Publish temperature: ");
      Serial.println(msg);
      client.publish("indoor/dht22/send/temperature", msg);
    }
}


// a function to be executed periodically
void readPir() {
    if(processing) return;
    long state = digitalRead(PIRPIN);
    if(state == HIGH) {
      Serial.println("Motion detected!");
      snprintf (msg, 75, "1");
      client.publish("indoor/pir/send/motion", msg);
      processing = true;
    } else {
      //Serial.println("Motion absent!");
      if(timeout) {
        Serial.println("Timeout activated");
        if(noMotionCtr == NO_MOTION_MAX) {
          Serial.println("Klling session!");
          snprintf (msg, 75, "0");
          client.publish("indoor/pir/send/motion", msg); 
          noMotionCtr = 0;
          timeout = false;
        } else {
          noMotionCtr++;
        }
      }
    }
    delay(1000);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(PIRPIN, INPUT);   // declare sensor as input
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  dht.begin();
  dhtTimer.setInterval(1800500, readDht);
  Serial.println("Calibrating...");
  delay(60000);
  Serial.println("Motion detection ready");
  pirTimer.setInterval(1000, readPir);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  dhtTimer.run();
  pirTimer.run();
}
