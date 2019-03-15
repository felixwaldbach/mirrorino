// Indoor sensor code

//Libraries
#include <DHT.h>;
#include <ArduinoJson.h>
#include <SoftwareSerial.h> // Only for USB
#include <Console.h> // For WiFi
#include <BridgeClient.h>
#include <Metro.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


//MQTT Setup
#define AIO_SERVER      "192.168.178.20"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "temperature"
#define AIO_KEY         "temperature"


//Constants
#define DHTPIN 2     // what pin we're connected to
#define PIRPIN 7     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)


//Variables and Objects
uint32_t zero = 0;
uint32_t pirvalue; //Stores pir value
uint32_t temp; //Stores temperature value
String data; //Stores dht values as json
uint32_t isUserDetected = 0;

boolean threadState1 = false;
boolean threadState2 = false;

Metro thread1 = Metro(1000);     // check every second
Metro thread2 = Metro(1800000);  // for dht22, 30 minutes

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

// Create a BridgeClient instance to communicate using the Yun's bridge & Linux OS.
BridgeClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Initialize Feeds for publishing
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/inside");
Adafruit_MQTT_Publish hcsrpir = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/pir");

void setup(){
  Serial.begin(9600);
  Bridge.begin();
  Console.begin();

  pinMode(PIRPIN, INPUT);     // declare sensor as inputPin

  dht.begin();

  // Send initial dht22 data
  readDHT();

  // One minute calibration time for pir
  delay(60000);

  Console.print("Indoor data");
  Console.println();
  Console.println();
  Console.println();
}

void loop(){
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    Console.println(F("MQTT Ping failed."));
  }

  if(thread1.check()) {
    readPIR();
  }
  if(thread2.check()) {
    readDHT();
  }
}

void readPIR() {

  pirvalue = digitalRead(PIRPIN);   // read sensor value
  if (pirvalue == 1) {              // check if the sensor is HIGH
    Console.println("Motion detected!");
    hcsrpir.publish(pirvalue);
  }
  else {
      Console.println("Motion stopped!");
      hcsrpir.publish(pirvalue);
      thread1.interval(1000); // set back
  }
}

void readDHT() {
    data = "";
    //Read data by using dht library and store it to variables hum and temp
    temp = dht.readTemperature();

    // Parse data into JSON
    root["name"] = "dht22";
    root["temp"] =  temp;
    root.printTo(data);

    // Check if any reads failed and exit early (to try again).
    if (isnan(temp)) {
      Console.println("Failed to read from DHT sensor!");
      return;
    } else {
      temperature.publish(temp);
      Console.println(data);
    }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Console.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Console.println(mqtt.connectErrorString(ret));
       Console.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  Console.println("MQTT Connected!");
}
