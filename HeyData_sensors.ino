//
//HeyData Sensors
//
//SensorBit (PCB026.*)
//SimpleSensor (PCB029.*)
//
//by Richard Hawthorn
//April 6th, 2016
//
//http://www.richardhawthorn.com
//http://www.deluxecapacitor.com
//http://www.heydata.co.uk
//

//make sure you rename and complete the connection_details.h file
//this defines wifi settings and all sensors
#include "connection_details.h"

// ------------------------------------

float value_float;
char value_char[10];
String value_string;

int minuteNow = 0;
int minuteLast = -1;

// ------------------------------------

#include <ESP8266WiFi.h>
#include <Wire.h>

#if sensor_temp
  #include "Adafruit_MCP9808.h"
  Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
#endif

#if sensor_humid
  #include "Adafruit_HTU21DF.h"
  Adafruit_HTU21DF htu = Adafruit_HTU21DF();
#endif

#if sensor_light
  #include <Adafruit_Sensor.h>
  #include <Adafruit_TSL2561_U.h>
  Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_LOW, 12345);
#endif

#if sensor_pressure
  #include <Adafruit_MPL3115A2.h>
  Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
#endif

#if sensor_temp
  #include "Adafruit_MCP9808.h"
  Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
#endif

#if sensor_temp_simple 
  #include "DHT.h"
  #define DHTPIN 14
  #define DHTTYPE DHT22
  DHT dht(DHTPIN, DHTTYPE);
#endif

void wifiConnect(){

  #if debug
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  #endif
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #if debug
      Serial.print(".");
    #endif
  }

  #if debug
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
  
}


void setup() {
  Serial.begin(115200);
  delay(10);

  #if sensor_humid
    if (!htu.begin()) {
      #if debug
        Serial.println("Error in humidity sensor setup");
      #endif
    }
  #endif

  #if sensor_light
    if(!tsl.begin()){
      #if debug
        Serial.println("Error in light sensor setup");
      #endif
    }
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
  #endif

  #if sensor_pressure
    if (!baro.begin()) {
      #if debug
        Serial.println("Error in pressure sensor setup");
      #endif
    }
  #endif

  #if sensor_temp
    if (!tempsensor.begin()) {
      #if debug
        Serial.println("Error in temperature sensor setup");
      #endif
    }
  #endif

  #if sensor_temp_simple
    dht.begin();
  #endif

  wifiConnect();
  
}

void sendValue(int id, float value){

  //convert float to string
  dtostrf(value, 5, 2, value_char);
  value_string = value_char;

  value_string.trim();

  #if debug
    Serial.print("connecting to ");
    Serial.println(host);
  #endif
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    #if debug
      Serial.println("connection failed");
    #endif
    return;
  }
  
  // We now create a URI for the request
  String url = "/api/v1/data/save";
  url += "?source_id=";
  url += id;
  url += "&token=";
  url += token;
  url += "&value=";
  url += value_string;

  #if debug
    Serial.print("Requesting URL: ");
    Serial.println(url);
  #endif
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(3000);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    #if debug
      Serial.print(line);
    #endif
  }

  #if debug
    Serial.println();
    Serial.println("closing connection");
  #endif
  
}

void checkHumid(){

   #if sensor_humid
    value_float = htu.readTemperature();
    sendValue(temp_id, value_float);

    #if debug
      Serial.print("Humidity value:");
      Serial.println(value_float);
    #endif

    #if sensor_humid_temp
      value_float = htu.readHumidity();
      sendValue(humid_id, value_float);

      #if debug
        Serial.print("Temperature value:");
        Serial.println(value_float);
      #endif
      
    #endif
  #endif  
}

void checkTemp(){
  #if sensor_temp
    value_float = tempsensor.readTempC();
    sendValue(temp_id, value_float);

    #if debug
      Serial.print("Temperature value:");
      Serial.println(value_float);
    #endif
    
  #endif

}

void checkLight(){
   #if sensor_light
    sensors_event_t event;
    tsl.getEvent(&event);
    value_float = event.light;

    if (value_float){
      sendValue(light_id, value_float);

      #if debug
        Serial.print("Light value:");
        Serial.println(value_float);
      #endif
    }
  #endif
}

void checkPressure(){
  #if sensor_pressure
    value_float = baro.getPressure() / 100000;
    sendValue(pressure_id, value_float);

    #if debug
      Serial.print("Pressure value:");
      Serial.println(value_float);
    #endif
    
  #endif
}

void checkSimpleTempHumid(){

  #if sensor_temp_simple 
    float value_humid = dht.readHumidity();
    float value_temp = dht.readTemperature();
    
    if (value_temp < 999){
      sendValue(temp_id, value_temp);
       #if debug
        Serial.print("Temperature value:");
        Serial.println(value_temp);
      #endif
    }

    #if sensor_humid_simple
      if (value_humid < 999){
         sendValue(humid_id, value_humid);
         #if debug
          Serial.print("Humidity value:");
          Serial.println(value_humid);
        #endif
      }
    #endif
  #endif
  
}

void loop() {

  minuteNow = round(millis() / 1000 / 60);

  if (minuteNow >= minuteLast + period){  

    minuteLast = minuteNow;

    checkTemp();
    checkHumid();
    checkLight();
    checkPressure();
    checkSimpleTempHumid();
    
  } else if (minuteNow < minuteLast){
    minuteLast = -1;
  }
  
  

}

