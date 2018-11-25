/*
  DHBW project

  Use esp8266 for wifi connection
  Send dht22 data via websockets to web UI every 30 minutes
  Use Softpot to toggle light and show current softpot value on web UI

  <Wiring>
  DHT22:
    1. 3,3V
    2. Pin we're connected to with 3,3V and 10K Resistor
    3. empty
    4. GND

  ESP8266:



*/

//Libraries
#include <DHT.h>;
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

//Constants
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define RX 10
#define TX 11
#define ESP8266
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
SoftwareSerial esp8266(RX,TX);

//Variables and Objects
int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value
String data;

String AP = "xy";       // CHANGE ME
String PASS = "xy"; // CHANGE ME
int countTrueCommand;
int countTimeCommand;
boolean found = false;

String HOST = "192.168.1.12";
String PORT = "5000";

StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();


void setup()
{
  Serial.begin(9600);

  esp8266.begin(115200);
  sendCommand("AT",5,"OK");
  sendCommand("AT+CWMODE=1",5,"OK");
  sendCommand("AT+CWJAP=\""+ AP +"\",\""+ PASS +"\"",20,"OK");

  dht.begin();
}

void loop()
{
    delay(2000);

    //Read data by using dht library and store it to variables hum and temp
    hum = dht.readHumidity();
    temp = dht.readTemperature();

    //Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");

    // Parse data into JSON
    root["name"] = "dht22";
    root["hum"] =  hum;
    root["temp"] =  temp;
    root.printTo(data);

    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    } else {
      //Print JSON to serial monitor
      Serial.println(data);

      // Send data via ESP8266

    }

    delay(10000); // Delay x sec. for next round
}

void sendCommand(String command, int maxTime, char readReplay[]) {
  Serial.print(countTrueCommand);
  Serial.print(". at command => ");
  Serial.print(command);
  Serial.print(" ");
  while(countTimeCommand < (maxTime*1))
  {
    esp8266.println(command);//at+cipsend
    if(esp8266.find(readReplay))//ok
    {
      found = true;
      break;
    }

    countTimeCommand++;
  }

  if(found == true)
  {
    Serial.println("Success");
    countTrueCommand++;
    countTimeCommand = 0;
  }

  if(found == false)
  {
    Serial.println("Fail");
    countTrueCommand = 0;
    countTimeCommand = 0;
  }

  found = false;
 }
