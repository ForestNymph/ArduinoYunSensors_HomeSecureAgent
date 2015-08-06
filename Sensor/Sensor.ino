#include<dht.h>
#include<YunClient.h>
#include<HttpClient.h>

#define DHT11_DIGITAL_PIN 4
#define LM35_ANALOG_PIN 1

dht DHT;

YunClient client;
const char* server_name="grudowska.pl";

Process process;
String packet = "";

// Create http request
String command = "curl -H ";
String content_type = "\"Content-Type: application/json\"";
String post_data_command = " -X POST -d ";
String start_payload = "'{\"sensorList\":[{";
String payload = "";
String end_payload = "}]}' ";
String adress = "grudowska.pl:8080";

void setup() {

  //start Bridge, and wait for a serial connection before going into loop()
  Bridge.begin();
  Serial.begin(9600);
  while (!Serial) ;

  Serial.print("Connecting...");
  if (client.connect(server_name, 80)) {
    Serial.println("Done!");
  } else {
    Serial.println("Fail");
    // Freeze on fail
    while (1);
  }
}


void loop() {
  packet = createHttpPacket();
  process.runShellCommand(packet);
  
  while (process.running());
  
  while (process.available()) {
    String result = process.readString();
    Serial.println(result);
  }
}

String createHttpPacket() {
  String payload = getPayload();
  String packet = command + content_type + post_data_command
  + start_payload + payload + end_payload + adress;
  return packet;
}

String getPayload() {
  String payload = "";
  
  float carbon = getCarbonMonoxide();
  payload += sensorNameValueToString("carbon monoxide", carbon);
  
  float distance = getDistance();
  payload += sensorNameValueToString("distance", distance);
  
  float gas = getGas();
  payload += sensorNameValueToString("gas", gas);
  
  float humidity = getHumidityDHT11();
  payload += sensorNameValueToString("humidity", humidity);
  
  float motion = getMotion();
  payload += sensorNameValueToString("motion", motion);
  
  float smoke = getSmoke();
  payload += sensorNameValueToString("smoke", smoke);
  
  float temperature = getTemperatureLM35();
  payload += sensorNameValueToString("temperature", temperature);
  
  float timestamp = getTimestamp();
  payload += sensorNameValueToString("timestamp", timestamp);
  
  delay(1000);
  
  return payload;
}

float getHumidityDHT11() {
  
  Serial.print("DHT11\t");
  
  int check = DHT.read11(DHT11_DIGITAL_PIN);
  switch (check) {
    case DHTLIB_OK: Serial.print("OK\t");
        break;
    case DHTLIB_ERROR_CHECKSUM: Serial.print("Checksum error\t");
        break;
    case DHTLIB_ERROR_TIMEOUT: Serial.print("Time out error\t");
        break;
    default: Serial.print("Unknown error\t");
        break;    
  }
  
  float hum = DHT.humidity;
  Serial.print("HUMIDITY = ");
  Serial.print(hum, 1);
  Serial.print("%\t");
  Serial.print("TEMPRATURE = ");
  Serial.print(DHT.temperature, 1);
  Serial.println("*C\t");
  
  return hum;
}

float getTemperatureLM35() {
  //convert the analog data to Celcius temperature
  float temp = (5.0 * analogRead(LM35_ANALOG_PIN) * 100.0) / 1024.0;//temp * 0.48828125;
  Serial.print("TEMPRATURE = ");
  Serial.print(temp);
  Serial.print("*C");
  Serial.println();
  
  return temp;
}

float getCarbonMonoxide() {
  return 0;
}

float getDistance() {
  return 0;
}

float getGas() {
  return 0;
}

float getMotion() {
  return 0;
}

float getSmoke() {
  return 0;
}

float getTimestamp() {
  return 0;
}

String sensorNameValueToString(String name, double value) {
  if (name == "timestamp") {
    return "\"" + name + "\"" + " : " + String(value) + " ";
  } else {
    return "\"" + name + "\"" + " : " + String(value) + ", ";
  }
}
