/*
Code combination February 2018, Nick van de Giesen 
Fast version

** Scope:
This program reads values from a BMP280 sensor and logs the values to the serial port (baud rate 9600)

** BMP280 
Uses BME280I2C.h 
Written by Tyler Glenn, 2016, see https://github.com/finitespace/BME280/tree/master/docs
GNU General Public License
Written: Dec 30 2015.
Last Updated: Oct 07 2017.

Connecting the BMP280 Sensor with Adafruit board on hardware address 0x76 (updated Hessel Winsemius, 20/02/18):
Sensor                    ->  Arduino
-------------------------------------
Vin (pin1)                ->  3.3V
Gnd (pin3)                ->  Gnd
SDA (Serial Data, pin17)  ->  A4 on Uno/Pro-Mini, 20 on Mega2560/Due, 2 Leonardo/Pro-Micro
SCK (Serial Clock, pin18) ->  A5 on Uno/Pro-Mini, 21 on Mega2560/Due, 3 Leonardo/Pro-Micro
SD0                       -> Gnd (forcing to address 0x76, if not connected, change address to 0x77 for I2C connection)

  created  24 Nov 2010
  modified 9 Apr 2012
  by Tom Igoe
 */

// import libraries
#include <SPI.h>  //https://www.arduino.cc/en/Reference/SPI
#include <Wire.h>
#include <BME280I2C.h>    //https://github.com/finitespace/BME280

// settings for logging and baud rate, reading as fast as possible (oversampling x1, forced mode)
#define SERIAL_BAUD 115200
BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_500us,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76 // I2C address. I2C specific.
);

// prepare bme object
BME280I2C bme(settings);    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

int counter = 0;

void setup () {
  // Open serial communications and wait for port to open:
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {} // Wait
  Wire.begin();
  while(!bme.begin())
  {
    Serial.println("Could not find BMP280 sensor!");
    delay(1000);
  }
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
  String header = "";
  header = "Temperature (°C),Pressure (Pa)";
  Serial.println(header);
}

void loop () {
//  String dataString = "";

  //Read & log sensor values fast
  while(counter < 10000){
    counter++;
    printBME280Data(&Serial);
  }

  while (1);    // Stop
}

void printBME280Data
(
   Stream* client
)
{
   float temp(NAN), hum(NAN), pres(NAN);

   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);

   bme.read(pres, temp, hum, tempUnit, presUnit);

//   client->print("Temp: ");
   client->print(temp);
//   client->print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F'));
//   client->print("\t\tHumidity: ");
//   client->print(hum);
//   client->print("% RH");
//   client->print("\t\tPressure: ");
   client->print(", ");
   client->println(pres);
//   client->println("Pa");

}


