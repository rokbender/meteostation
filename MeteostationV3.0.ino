extern "C" {
    #include "user_interface.h"  // Required for wifi_station_connect() to work
}

#include <MQTT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#include <GyverBME280.h>
GyverBME280 bme;
#include <Arduino.h>
#include <BH1750FVI.h>
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 14 // Пин подключения OneWire шины, 13 (D7)
OneWire oneWire(ONE_WIRE_BUS); // Подключаем бибилотеку OneWire
DallasTemperature sensors(&oneWire); // Подключаем бибилотеку DallasTemperature

#include "DHT.h"
#define DHTTYPE DHT22
uint8_t DHTPin = 12; 
DHT dht(DHTPin, DHTTYPE);                
float Humidity;

#define Switch 13

const char *ssid =  "";  
const char *pass =  ""; 

const char *mqtt_server = ""; 
const int mqtt_port = 1883; 
const char *mqtt_user = ""; 
const char *mqtt_pass = ""; 

#define BUFFER_SIZE 100



int attempsWifi=0;
int attempsMQTT=0;
int attemps=2;

uint16_t lux;
float tempOut;
float tempInBox;
float pressure;
float HPaPressure;
float SolRad;

float vout = 0.0;           //
float vin = 0.0;            //
float R1 = 333500.0;          // опір R1 (10K)
float R2 = 97400.0;         // опір R2 (1,5K)
int val=0;
#define analogvolt A0

// Функція отримання даннихв ід сервера
//weather cloud
const char* server2 = "api.weathercloud.net";
char ID2 [] = "";
char Key2 [] = "";  
bool flag = true;



void callback( const MQTT ::Publish& pub)
{
  Serial.print(pub.topic());   // выводим в сериал порт название топика
  Serial.print(" => ");
  Serial.print(pub.payload_string()); // выводим в сериал порт значение полученных данных
  
  String payload = pub.payload_string();
  
   
}



WiFiClient wclient;      
PubSubClient client(wclient, mqtt_server, mqtt_port);

void setup() {
  delay(10);
  WiFi.persistent(false);  //disable saving wifi config into SDK flash area
  WiFi.disconnect(false);
  WiFi.forceSleepBegin();
  pinMode(Switch,OUTPUT);
  digitalWrite(Switch,HIGH); 
  Serial.begin(115200);
  Serial.println("tyt");
  delay(100);
  LightSensor.begin();
  sensors.begin(); // Иницилизируем датчики
  delay(300);
  dht.begin();
  // запуск датчика и проверка на работоспособность
  if (!bme.begin(0x76)) Serial.println("Error!");
}

void loop() {
  ReadSensors();
  
  // подключаемся к wi-fi
  
  
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);
    if (attempsWifi>attemps){
    Sleep1();
    }
    if (WiFi.waitForConnectResult() != WL_CONNECTED){
       delay(250);
       attempsWifi+=1;
       return;
      }
      
    Serial.println("WiFi connected");
  }
  if(flag){
    WeatherCloud();
    flag = false;
  }
  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect("Meteostation") // назва пристрою що підкл тобто ESp8266
                         .set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Connected to MQTT server");
        client.set_callback(callback);
      } else {
        Serial.println("Could not connect to MQTT server");
        attempsMQTT+=1;
        delay(250);
        if (attempsMQTT>attemps){
           Sleep1();
          }   
      }
    }

    if (client.connected()){
      client.loop();
      SendData();
      delay(20);
      Sleep1();
  }
  
}

} // конец основного цикла

void ReadSensors(){
  delay(50);
  lux = LightSensor.GetLightIntensity();
 // lux=5;
  Serial.println(" ");
  Serial.print("Освещенность: ");
  Serial.print(lux);
  Serial.println(" lX");
  SolRad=lux*0.079;
  Serial.print("SolarRadiation=");
  Serial.println(SolRad);
  
  sensors.requestTemperatures();
  delay(1000);
  tempOut = sensors.getTempCByIndex(0);
  Serial.print("temperature ds18b20: ");
  Serial.println(tempOut);
  
  
  
  Humidity = dht.readHumidity();       // получить значение влажности
  Serial.print("humidity dht22: ");
  Serial.println(Humidity);
  
  // температура
  Serial.print("Temperature BMP280: ");
  tempInBox=bme.readTemperature();
  Serial.println(tempInBox);
  Volt() ;
  // давление
  Serial.print("Pressure: ");
  pressure = bme.readPressure();
  Serial.print("PressureHPA: ");  //тиск в паскалях
  HPaPressure=pressure;
  
  Serial.println(HPaPressure);
  pressure=pressureToMmHg(pressure);
  Serial.println(pressure); 
  
}
// Функция отправки показаний с термодатчика
void SendData(){
    
    client.publish("meteostation/voltage",String(vin));
    delay(10);
    client.publish("meteostation/tempOut",String(tempOut)); // отправляем в топик для термодатчика значение температуры
    delay(10);
    client.publish("meteostation/tempIn",String(tempInBox)); // отправляем в топик для термодатчика значение температуры
    delay(10);
    client.publish("meteostation/humid",String(Humidity)); // отправляем в топик для термодатчика значение температуры
    delay(10);
    client.publish("meteostation/pressure",String(pressure)); // отправляем в топик для термодатчика значение температуры
    delay(10);
    client.publish("meteostation/bright",String(lux)); // отправляем в топик для термодатчика значение температуры
    delay(300);
    Serial.println("SEND...");
   
}
void Sleep1(){
  Serial.println("SleepNow");
  digitalWrite(Switch,LOW);
  delay(100); 
  ESP.deepSleep(12e8);
  //ESP.deepSleep(10e6);
  }
void Volt(){
  delay(100);
  val = analogRead(analogvolt);
  Serial.println(val);
  if (val>0){
     vout = (val * 1.0) / 1024.0;
     vin = (vout / (R2 / (R1 + R2)))*0.97;
    }
   else{
    vin=0.0;
    } 
 
  Serial.println(vin);
}
void WeatherCloud(){

  Serial.print("connecting to ");
  Serial.print(server2);
  Serial.println("...");
  WiFiClient client;
  if (client.connect(server2, 80)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
  } else {
    Serial.println("connection failed");
  }
  client.print("GET /set/wid/");
  client.print(ID2);
  client.print("/key/");
  client.print(Key2);
  //tempOut=10.23;
  if (tempOut!=-127){
      client.print("/temp/");
     //tempOut=16.5;
     Serial.println(((int)(tempOut * 10)));
     client.print(((int)(tempOut * 10)));
    }
    
  if(isnan(Humidity)==false){
      client.print("/hum/");
      //Humidity=60.6;
      client.print(Humidity);
    }
  
  client.print("/bar/");
  //pressure=1020.1782;
  client.print(((int)(HPaPressure/10)));
  Serial.println(((int)(HPaPressure/10)));
  client.print("/solarrad/");
  Serial.println(((int)(SolRad * 10)));
  client.print(((int)(SolRad * 10)));
 
  client.println("/ HTTP/1.1");
  client.println("Host: api.weathercloud.net");
  client.println();
  delay(1000);
 
  if (!client.connected()) {
    Serial.println("client disconected.");
      if (client.connect(server2, 80)) {
    delay(100);
    Serial.print("connected to ");
    Serial.println(client.remoteIP());

  } else {
    Serial.println("connection failed");
    }
  }

 
}
