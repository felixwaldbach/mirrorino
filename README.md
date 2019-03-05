# mirrorino

## What does this Repository contain?
This repository holds code for the Arduino Yun which connects to the Magic Mirror. This application is written in the C language has two tasks:
1. Reading the DHT22 sensor on pin 2 and publishing the data every 30 minutes via MQTT to the Raspberry Pi Broker.
2. Motion detection with the HC-SR501 PIR sensor on pin 7 to trigger the face recognition process.

## Getting started
First of all, ensure to have a running Arduino environment. Make sure you have installed following libraries (Search Online...):
```
1. DHT.h
2. Metro.h
3. Adafruit_MQTT.h
4. Adafruit_MQTT_Client.h
```

Clone this package and distribute it on the Arduino Yun. Then, change the following line in the code:
```
#define AIO_SERVER      "IP of your Broker"
```

## Problems
The HC-SR501 PIR sensor is not working correctly why there is a delay of 3 minutes after the detection of a motion to give the user a 3 minute time slot for his interaction with the Smart Mirror.
Else, the HC-SR501 PIR sensor will try to detect a motion every second and send a "0" to the Broker. Only a detected motion will send a "1" to the Broker.

## Contributors
This project is developed and maintained by [Emre Besogul](https://github.com/emrebesogul) and [Felix Waldbach](https://github.com/felixwaldbach).
For questions reach out to us!
