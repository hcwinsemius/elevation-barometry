/*
Code combination April 2018, Hessel Winsemius, Deltares
Fast version

** Scope:
This program reads values from a BMP280 sensor and logs the values to the serial port (baud rate 9600)
The pressure readings are averaged over 150 samples so that a better representative mean is achieved.

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
#include <EnvironmentCalculations.h>
#include <BME280I2C.h>    //https://github.com/finitespace/BME280
#include "RunningStats.h"

// settings for logging and baud rate, reading as fast as possible (oversampling x1, forced mode)
#define SERIAL_BAUD 9600

float referencePressure = 100000;  // hPa local QFF (official meteor-station reading)
RunningStats rs_pres;
RunningStats rs_temp;
RunningStats rs_altitude;

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
int max_samples = 500; // maximum amount of samples before an average is returned to the serial port
int flag = 0; // flag indicating if an acceptable standard deviation is reached within max_samples (0) or not (1)
int done = 0; // flag indicating if a value was returned, continue with new reading (0) or add more samples to current (1)
float pres = 0; // instantaneous pressure
float hum = 0; // instantaneous humidity (not used)
float temperature = 0; // instantaneous temperature
float altitude = 0; // instantaneous calculated elevation

float altitude_mean = 0; // mean altitude [m]
float altitude_std = 0; // standard deviation altitude
float pres_mean = 0; // mean pressure [Pa]
float pres_std = 0; // standard deviation of pressure
float temp_mean = 0; // mean temperature [oC]
float temp_std = 0; // standard deviation temperature
float pres_std_thres = 3.5;  // acceptable standard deviation [Pa] after which a mean value is returned

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
  header = "Temperature (C), Pressure (Pa), Elevation (m), Temp. stdev [C], Pres. stdev [Pa], Elev. stdev [m], flag, sample count [-]";
  Serial.println(header);
  // INITIALIZATION
  // first read 1000 values and discard them (seems that sensor needs some initialization
  while(counter < 1000)
  {
    BME280Data();
    counter++;
  }
}

void loop () {
  // clear the running mean object for pressure
  int done = 1;
  float pres_std = 9999;
  int counter = 0;
  rs_temp.Clear();
  rs_pres.Clear();
  rs_altitude.Clear();
  // a new read is performed below, by sampling several values (at least 150) and averaging these
  while(done == 1)
  {
    counter++;
    BME280Data();
    // add pressure reading to averaging object
    rs_pres.Push(pres);
    rs_temp.Push(temperature);
    rs_altitude.Push(altitude);
    float pres_mean = rs_pres.Mean();
    float pres_std = rs_pres.StandardDeviation();
    float temp_mean = rs_temp.Mean();
    float temp_std = rs_temp.StandardDeviation();
    float altitude_mean = rs_altitude.Mean();
    float altitude_std = rs_altitude.StandardDeviation();
    delay(1);
    // we want at least one second of data (at 150Hz)
    if(counter > 150)
    {
      if(pres_std < pres_std_thres)
      {
        flag = 0;  // we have a satisfactory st. dev.
        done = 0;  // the read is done
        // print to serial port
        printBMPData(temp_mean, pres_mean, altitude_mean, temp_std, pres_std, altitude_std, flag, counter);
      }
      if(counter > max_samples)
      {
        flag = 1;  // we reached the maximum amount of samples, return with a flag
        done = 0;  // the read is done
        // print to serial port
        printBMPData(temp_mean, pres_mean, altitude_mean, temp_std, pres_std, altitude_std, flag, counter);
      }
    }
  }
}

void BME280Data()
{
   // set units
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);
   
   // read data with given settings
   bme.read(pres, temperature, hum, tempUnit, presUnit);

   // calculate elevation
   EnvironmentCalculations::AltitudeUnit envAltUnit  =  EnvironmentCalculations::AltitudeUnit_Meters;
   EnvironmentCalculations::TempUnit     envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;

   // To get correct local altitude/height (QNE) the reference Pressure
   // should be taken from meteorologic messages (QNH or QFF)
   altitude = EnvironmentCalculations::Altitude(pres, envAltUnit, referencePressure, temperature, envTempUnit);
}

void printBMPData(float temp_mean, float pres_mean, float altitude_mean, float temp_std, float pres_std, float altitude_std, int flag, int counter)
{
  // print all required data into a nice Serial print statement
  Serial.print(temp_mean);
  Serial.print(", ");
  Serial.print(pres_mean);
  Serial.print(", ");
  Serial.print(altitude_mean);
  Serial.print(", ");
  Serial.print(temp_std);
  Serial.print(", ");
  Serial.print(pres_std);
  Serial.print(", ");
  Serial.print(altitude_std);
  Serial.print(", ");
  Serial.print(flag);
  Serial.print(", ");
  Serial.println(counter);
  return;
}

// OLD STUFF
//    presAv.addValue(pres);
     // Push values of pressure to rs object
//     float pres_average = presAv.getAverage();

  
  // WINDOW FILLING
  // first fill up the averaging window completely
//  while(counter < movAv)
//  {
//    BME280Data();
//    presAv.addValue(pres);
//    counter++;
//  }
  // now consecutively add and replace a value, and use the means to develop a time series
//  while(counter < amount_samples)


