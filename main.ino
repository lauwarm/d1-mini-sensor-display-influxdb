#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <ESP8266WiFiMulti.h>
#include <LiquidCrystal.h>
#include "DHT.h"

ESP8266WiFiMulti wifiMulti;

#define DHTTYPE DHT22

#define WIFI_SSID ""
#define WIFI_PASS ""

#define DEVICE ""
#define INFLUXDB_URL ""
#define INFLUXDB_TOKEN ""
#define INFLUXDB_ORG ""
#define INFLUXDB_DB_NAME ""
#define INFLUXDB_USER ""
#define INFLUXDB_PASSWORD ""

#define GPIO02 2

//pinMode(GPIO02, OUTPUT);
//digitalWrite(GPIO02, HIGH);

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point sensor("wifi status");

uint8_t DHTPin = 0;

DHT dht(DHTPin, DHTTYPE);

LiquidCrystal lcd(5, 4, 2, 14, 12, 13); // Creates an LC object. Parameters: (rs, enable, d4, d5, d6, d8)

unsigned long previous_time = 0;
unsigned long delay_time = 20000;

void setup() {
  Serial.begin(9600);
//  Serial.println();

//  pinMode(LED_BUILTIN, OUTPUT); // no need, remove led
//  digitalWrite(LED_BUILTIN, HIGH); // no need, remove led

  WiFi.disconnect(true);

  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to: ");
  Serial.print(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP Address:");
  lcd.setCursor(0, 1); ///  Move to Next line
  lcd.print(WiFi.localIP());

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);

  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {
  unsigned long current_time = millis();

  if ((WiFi.status() != WL_CONNECTED) && (current_time - previous_time >= delay_time)) {
    Serial.println(millis());
    Serial.println("Reconnecting to WiFi");
    WiFi.disconnect();
    WiFi.reconnect();
    previous_time = current_time;
  }

  // Store measured value into point
  sensor.clearFields();
  // Report RSSI of currently connected network
  sensor.addField("rssi", WiFi.RSSI());

  double dht22_temperature = dht.readTemperature();
  double dht22_humidity = dht.readHumidity();

  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print(millis());

  lcd.setCursor(0,1);
  lcd.print("C");
  lcd.print((char)223);
  lcd.print(": ");
  lcd.setCursor(3, 1);
  lcd.print(dht22_temperature, 1);
  lcd.setCursor(9,1);
  lcd.print("H%: ");
  lcd.setCursor(12, 1);
  lcd.print(dht22_humidity), 1;

  if(isnan(dht22_temperature)) {
    Serial.println("Read Failure DHT22 Temperature!");
  } else {
    sensor.addField("temperature", dht22_temperature);
  }

  if(isnan(dht22_humidity)) {
    Serial.println("Read Failure DHT22 Humidity!");
  } else {
    sensor.addField("humidity", dht22_humidity);
  }
  
  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  //Wait 10s
  Serial.print("Timer: ");
  Serial.println(millis());
  delay(10000);
//  ESP.deepSleep(60e6);
}
