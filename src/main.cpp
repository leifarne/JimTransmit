// rf95_reliable_datagram_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_server
// Tested with Anarduino MiniWirelessLoRa, Rocket Scream Mini Ultra Pro with the RFM95W

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <Adafruit_SleepyDog.h>

float readTemp();
float readBattery();
bool transmitValue(float t, float vbat);
void blink(int t);


#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

/* for feather32u4
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
*/

/* for feather m0 */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(rf95, CLIENT_ADDRESS);

// Need this on Arduino Zero with SerialUSB port (eg RocketScream Mini Ultra Pro)
//#define Serial SerialUSB

#define LED 11
#define VBATPIN A7
#define PWR_DONE A2

#define BLINK_VERY_FAST 50
#define BLINK_FAST 200
#define BLINK_SLOW 1000

int blinkTime = 0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(12);
DallasTemperature sensors(&oneWire);


void setup()
{
  // Ensure serial flash is not interfering with radio communication on SPI bus
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  pinMode(PWR_DONE, OUTPUT);
  digitalWrite(PWR_DONE, LOW);

  Serial.begin(9600);
  Serial.println("Welcome to another day");
  // while (!Serial) ; // Wait for serial port to be available

  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
//  driver.setTxPower(23, false);
  // If you are using Modtronix inAir4 or inAir9,or any other module which uses the
  // transmitter RFO pins and not the PA_BOOST pins
  // then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true.
  // Failure to do that will result in extremely low transmit powers.
//  driver.setTxPower(14, true);
  // You can optionally require this module to wait until Channel Activity
  // Detection shows no activity on the channel before transmitting by setting
  // the CAD timeout to non-zero:
//  driver.setCADTimeout(10000);

}


float readTemp() {
  Serial.print("Requesting temperaturesa...");
  sensors.begin();
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float t = sensors.getTempCByIndex(0);
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(t);

  return t;
}


float readBattery() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  Serial.print("VBat: " ); Serial.println(measuredvbat);
  return measuredvbat;
}


//uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

void loop()
{
  blinkTime = 0;

  float temp = readTemp();
  float vbat = readBattery();

  if (vbat < 2.8) {
    blinkTime = BLINK_SLOW;
  }

  // Send a message to manager_server
  Serial.println("Sending to rf95_reliable_datagram_server");
  String msg = String(temp, 2) + "; " + String(vbat, 2);
  Serial.println(msg);
  if (manager.sendtoWait( (uint8_t *) msg.c_str(), msg.length(), SERVER_ADDRESS ))
  {
    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
    {
      Serial.print("got reply from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);

      // System power down!
      digitalWrite(PWR_DONE, HIGH);
      delay(500);
      // We never get here.
    }
    else
    {
      Serial.println("No reply, is rf95_reliable_datagram_server running?");
      blinkTime = BLINK_FAST;
    }
  }
  else {
    Serial.println("sendtoWait failed");
    blinkTime = BLINK_FAST;
  }

  blink(blinkTime);

  delay(500);
}

void blink(int t) {
  if (t == 0) {
    return;
  }

  for (int i = 0; i < 5; i++) {
    digitalWrite(LED, HIGH);
    delay(t);
    digitalWrite(LED, LOW);
    delay(t);
  }
}
