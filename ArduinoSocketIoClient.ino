/*
 * WebSocketClientSocketIO.ino
 *
 *  Created on: 06.06.2016
 *
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <WebSocketsClient.h>

#include <Hash.h>

#include "DHT.h"
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

bool isConnected = false;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    Serial.printf("Receiving event...");

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected!\n");
            isConnected = false;
            break;
        case WStype_CONNECTED:
            {
                Serial.printf("[WSc] Connected to url: %s\n",  payload);
                isConnected = true;
                digitalWrite(13, LOW); //Led port ausschalten
                delay(1000); //1 Sek Pause
                digitalWrite(13, HIGH); //Led port einschlaten
                delay(1000);
			          // send message to server when Connected
                // socket.io upgrade confirmation message (required)
				        webSocket.sendTXT("5");
            }
            break;
        case WStype_TEXT:
            Serial.printf("[WSc] get text: %s\n", payload);
            break;
        case WStype_BIN:
            Serial.printf("[WSc] get binary length: %u\n", length);
            hexdump(payload, length);
            break;
    }

}

void setup() {
  dht.begin();

    Serial.begin(115200);

    Serial.printf("Connecting to wifi..");
    WiFiMulti.addAP("WIFI_NAME", "PW");
    digitalWrite(13, HIGH);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }


    Serial.printf("Connecting to socket io server..");
    webSocket.beginSocketIO("192.168.0.103", 5000);
    // example socket.io message with type "messageType" and JSON payload
    webSocket.sendTXT("42[\"message\",{\"message\":\"hello from arduino\"}]");
    //webSocket.setAuthorization("user", "Password"); // HTTP Basic Authorization
    webSocket.onEvent(webSocketEvent);

}

void loop() {

  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  webSocket.sendTXT("42[\"message\",{\"temperature\":\"%f\"}]", h);

    webSocket.loop();

    if(isConnected) {
    }
}
