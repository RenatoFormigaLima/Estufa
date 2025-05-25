#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>

#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESPAsyncWebServer.h>

#ifdef AP_SECURITY
  #define AP_PASSW "123!@#"
#endif

#define AP_NAME "ESTUFA-ESP32"

#define PIN_LCD_1 13
#define PIN_LCD_2 12
#define PIN_LCD_3 14

#define DHT11_PIN 32
#define DHTTYPE DHT11

#define PIN_SOIL_1 27
#define PIN_SOIL_2 26

#define PIN_RELE_1 25
#define PIN_RELE_2 33

#define VAZAO_BOMBA_l_H  240
#define VAZAO_BOMBA_ml_s ((VAZAO_BOMBA_L_H)/3600)*1000

#define SOIL_SENSORS 1
#define SOIL_TARGET 50
#define SOIL_LOW 20
#define SOIL_HIGH SOIL_TARGET*1.5

#define TEMP_SENSORS 1
#define TEMP_TARGET 30
#define TEMP_HIGH TEMP_TARGET*1.3


enum RELESTATE
{
  ON = 0x00,
  OFF
};

enum device_type
{
  sensor,
  lcd,
  rele
};

struct device
{
  int type;
  int id;
  int value;
} typedef device;

DHT dht(DHT11_PIN, DHTTYPE);

//ESP SERVER
AsyncWebServer server(80);

device* temp_sensors = new device[TEMP_SENSORS];
device* soil_sensors = new device[SOIL_SENSORS];

device rele01 = {device_type::rele, 0, RELESTATE::OFF};
device rele02 = {device_type::rele, 1, RELESTATE::OFF};

int soil_value;
int temp_value;

void ChangeReleState(int id, int state);
void printStatus();

void setup() {

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  #ifdef AP_SECURITY
    WiFi.softAP(AP_NAME, AP_PASSW);
  #else
    WiFi.softAP(AP_NAME);
  #endif
  if(!WiFi.begin())
    esp_restart();

  for(int i = 0; i < TEMP_SENSORS; i++){
    temp_sensors[i].id = i;
    temp_sensors[i].type = device_type::sensor;
    temp_sensors[i].value = 0;
  }
  for(int i = 0; i < SOIL_SENSORS; i++){
    soil_sensors[i].id = i;
    soil_sensors[i].type = device_type::sensor;
    soil_sensors[i].value = 0;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
    req->send(SPIFFS, "/index.html");
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req){
    float temp_cel = dht.readTemperature();
    float humidity = dht.readHumidity();
    if(isnan(temp_cel))
    {
      temp_cel = -1;
    }
    if(isnan(humidity))
    {
      humidity = -1;
    }
    String body = "{\n\"Temp\":" + String(temp_cel) + ",\n\"humidity\":"+String(humidity)+",\n\"soil\":" + String(soil_value) + ",\n\"rele01\":" + String(rele01.value) +",\n\"rele02\":"+ String(rele02.value) +"\n}";
    req->send(200, "application/json", body);
  });

  server.on("/setpoint", HTTP_POST, [](AsyncWebServerRequest* req){
    String data = String(TEMP_TARGET);
    if(req->hasParam("setpoint"))
        data = req->getParam("setpoint")->value();
    req->send(200);
  });
  server.begin();
}

void loop() {
  
  if(soil_value <= SOIL_HIGH && soil_value >= SOIL_TARGET)
  {
    ChangeReleState(0, RELESTATE::OFF);
  }
  if(soil_value < SOIL_TARGET)
  {
    ChangeReleState(0, RELESTATE::ON);
  }
  if(temp_value < TEMP_TARGET)
  {
    ChangeReleState(1, RELESTATE::OFF);
  }
  if(temp_value >= TEMP_TARGET && temp_value <= TEMP_HIGH)
  {
    ChangeReleState(1, RELESTATE::ON);
  }

  printStatus();
}

