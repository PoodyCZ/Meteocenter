#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_SHT31.h>

Adafruit_BMP085 tlakomer;
Adafruit_SHT31 teplomer = Adafruit_SHT31();

int stationid = 1;

// Promenne meteorologickych udaju
float teplota = 0.0f, teplota1 = 0.0f, tlak = 0.0f;
uint8_t vlhkost = 0;

// Prihlasovaci udaje k Wi-Fi
const char ssid[] = "ssid";
const char heslo[] = "pass";

int sleepTimeS = 60; // Doba uspani v sekundach:

void odesliDoDatabaze();
void pripojit();
void aktualizace();
void ziskejHodnoty();

void setup() {
  Serial.begin(115200);
  if (!tlakomer.begin()) {
    Serial.println("Tlakomer neodpovida. Zkontroluj zapojeni!");
    while (1) {}
  }

  if (!teplomer.begin(0x45)) {
    Serial.println("Teplomer neodpovida. Zkontroluj zapojeni!");
    while (1) {}
  }

  Serial.println("Stanice spustena!");
  pinMode(D0, WAKEUP_PULLUP);
  pripojit();
  aktualizace();
  ziskejHodnoty();
  odesliDoDatabaze();
  Serial.println("Jdu spat!");
  ESP.deepSleep((sleepTimeS * 1000000), WAKE_RF_DEFAULT);
}

void loop() {
}

void pripojit() {  
  WiFi.hostname("meteostanice");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, heslo);
  Serial.println("Pripojuji se k Wi-Fi siti");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("");
}

void aktualizace() {
  Serial.println("Zacinam aktualizaci z DB");
  WiFiClient client;
  HTTPClient http;
  String url = "http://192.168.0.5/api/api.php?stationid=";
  url = url + stationid;
  http.begin(client, url.c_str());
  int httpCode = http.GET();
  if (httpCode > 0) { 
    String payload = http.getString();
    DynamicJsonDocument data(1024);
    deserializeJson(data, payload);
    sleepTimeS = data["data"]["sleeptime"];
  }
  http.end();
}

void odesliDoDatabaze()
{
  Serial.println("Spoustim odesilani do db");
  DynamicJsonDocument data(1024);
  data["stationid"] = stationid;
  data["temperature"] = teplota;
  data["pressure"] = tlak;
  data["humidity"] = vlhkost;

  String datastring;
  serializeJson(data, datastring);

  WiFiClient client;
  HTTPClient http;
  http.begin(client, "http://192.168.0.5/api/api.php");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(datastring);
  if (httpCode > 0) { 
    String payload = http.getString();
    deserializeJson(data, payload);
    const char* recv = data["data"];
    Serial.println(recv);
  }
  http.end();
  WiFi.disconnect();
  Serial.println("Konec odesilani do db");
}

// Funkce pro precteni hodnot ze senzoru do promennych
void ziskejHodnoty() {
  tlak = tlakomer.readPressure() / 100.0f;
  teplota = teplomer.readTemperature();
  vlhkost = teplomer.readHumidity();
}