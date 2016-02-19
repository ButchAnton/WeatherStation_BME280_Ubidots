/*

WeatherStation.ino
Butch Anton
butch@butch.net
@ButchAnton

Gather various data from the BME280 and post it to the cloud using a Particle Photon.
This code should work with any Arduino like device, though obviously the Wi-Fi bit
and perhaps the I2C pins may need to change.

In this particular case, the data gathered is temperature, barometric
pressure, relative humidity, altitude, and dew point.  This date is then
posted every second to a Ubidots instance.

Connecting the BME280 Sensor:
Sensor              ->  Board
-----------------------------
Vin (Voltage In)    ->  3.3V
Gnd (Ground)        ->  Gnd
SDA (Serial Data)   ->  D0 on Particle Photon
SCK (Serial Clock)  ->  D1 on Particle Photon

 */

#include "application.h"
#include <BME280.h>
#include <OneWire.h>
#include <stdarg.h>
#include "HttpClient.h"

#define SERIAL_BAUD 115200

BME280 bme;                   // Default : forced mode, standby time = 1000 ms
                              // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
bool metric = false;          // If you want metric values, change this to true.

// Ubidots API information

const String temperature_variable_id = "56be583e76254213be6afc13";
const String humidity_variable_id = "56be58477625421455a80019";
const String pressure_variable_id = "56be58527625421455a80037";
const String altitude_variable_id = "56be58667625421455a80049";
const String dewpoint_variable_id = "56be586c7625421451563567";
const String ubidots_token = "UL7wpuTgnlsn04DwPFmytrr3cX5kDq";

// Do something a bit different -- post all the data as a collection.

String server = "things.ubidots.com";
int port = 80;
char *path = "/api/v1.6/collections/values";
HttpClient http;
// Set the content type and the Ubidots authentication token.
http_header_t headers[] = {
    { "Content-Type", "application/json" },
    { "X-Auth-Token", ubidots_token },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

http_request_t request;
http_response_t response;

void printBME280Data(Stream * client);

// A couple of utility functions that sort of work like a general
// printf statement.  Sort of.

void pnprintf(Print& p, uint16_t bufsize, const char* fmt, ...) {
   char buff[bufsize];
   va_list args;
   va_start(args, fmt);
   vsnprintf(buff, bufsize, fmt, args);
   va_end(args);
   p.println(buff);
}

#define pprintf(p, fmt, ...) pnprintf(p, 128, fmt, __VA_ARGS__)
#define Serial_printf(fmt, ...) pprintf(Serial, fmt, __VA_ARGS__)

void examples() {
   pnprintf(Serial, 64, "%s", "2123");
   pprintf(Serial, "%d", 123);
   Serial_printf("%d", 123);
}

// Post data to Ubidots.  This is pretty generic.  Just contruct a properly formatted JSON body
// and send it via HTTP POST.

void postDataToCloud(float temperature, float humidity, float pressure, float altitude, float dewpoint) {

  request.hostname = server;
  request.port = port;
  request.path = path;
  Serial_printf("******* POSTing weather data: t = %f\th = %f\tp = %f\ta = %f\td = %f", temperature, humidity, pressure, altitude, dewpoint);
  String body = "";
  body += "[";
  body += "{\"variable\": \"" + temperature_variable_id + "\", \"value\": " + temperature + "}, ";
  body += "{\"variable\": \"" + humidity_variable_id + "\", \"value\": " + humidity + "}, ";
  body += "{\"variable\": \"" + pressure_variable_id + "\", \"value\": " + pressure + "}, ";
  body += "{\"variable\": \"" + altitude_variable_id + "\", \"value\": " + altitude + "}, ";
  body += "{\"variable\": \"" + dewpoint_variable_id + "\", \"value\": " + dewpoint + "}";
  body += "]";
  Serial.print("body = "); Serial.println(body);
  request.body = body;

  http.post(request, response, headers);
  Serial.print("#### Response code = "); Serial.print(response.status);
  Serial.print(" , body = "); Serial.println(response.body);
  Serial.println("*******");
}

void printBME280Data(Stream* client){
  float temp(NAN), hum(NAN), pres(NAN), altitude(NAN), dewPoint(NAN);
  uint8_t pressureUnit(3);                                           // unit: B000 = Pa, B001 = hPa, B010 = Hg, B011 = atm, B100 = bar, B101 = torr, B110 = N/m^2, B111 = psi
  bme.ReadData(pres, temp, hum, metric, pressureUnit);               // Parameters: (float& pressure, float& temp, float& humidity, bool celsius = false, uint8_t pressureUnit = 0x0)
  altitude = bme.CalculateAltitude(metric);
  dewPoint = bme.CalculateDewPoint(metric);
  postDataToCloud(temp, hum, pres, altitude, dewPoint);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {} // Wait
  while(!bme.begin()){
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }
  request.path = "/api/v1.6/collections/values";
}

void loop() {
   printBME280Data(&Serial);
   delay(1000);
}
