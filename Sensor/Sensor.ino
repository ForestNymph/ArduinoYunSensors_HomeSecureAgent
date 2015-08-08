#include<dht.h>
#include<MQ2.h>
#include<YunClient.h>

#define DHT11_DIGITAL_PIN 4
#define LM35_ANALOG_PIN 1
#define HCSR501_DIGITAL_PIN 7
#define MQ2_ANALOG_PIN 2

dht DHT;
MQ2 mq2;

YunClient client;
const char* server_name = "grudowska.pl";

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
  Serial.print("Calibrating...\n");
  mq2.Ro = MQCalibration(MQ2_ANALOG_PIN);
  Serial.print("Calibration is done.\n");
}

void loop() {
  packet = createHttpPacket();
  process.runShellCommand(packet);

  // Serial.print("Ro=");
  // Serial.print(mq2.Ro);
  // Serial.print(" kohm");
  // Serial.print("\n");

  while (process.running());

  while (process.available()) {
    String result = process.readString();
    Serial.println(result);
  }
  delay(1000);
}

String createHttpPacket() {
  String payload = getPayload();
  String packet = command + content_type + post_data_command
                  + start_payload + payload + end_payload + adress;
  return packet;
}

String getPayload() {
  String payload = "";

  float carbon = get_carbonmonoxide_MQ9();
  payload += to_string("carbon monoxide", carbon);

  float gas = get_gas_MQ2();
  payload += to_string("gas", gas);

  float humidity = get_humidity_DHT11();
  payload += to_string("humidity", humidity);

  float motion = get_motion_HCSR501();
  payload += to_string("motion", motion);

  float smoke = get_smoke_MQ2();
  payload += to_string("smoke", smoke);

  float temperature = get_temperature_LM35();
  payload += to_string("temperature", temperature);
  
  // Serial.println(payload);

  return payload;
}

float get_humidity_DHT11() {
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

float get_temperature_LM35() {
  //convert the analog data to Celcius temperature
  //float temp = (5.0 * analogRead(LM35_ANALOG_PIN) * 100.0) / 1024.0;//temp * 0.48828125;
  float temp = DHT.temperature;
  // Serial.print("TEMPRATURE = ");
  // Serial.print(temp);
  // Serial.print("*C");
  // Serial.println();

  return temp;
}

// If only Carbon Monoxide is tested, the heater can be set at 1.5V.
float get_carbonmonoxide_MQ9() {
  float value = mq2.MQGetGasPercentage(MQRead(analogRead(MQ2_ANALOG_PIN)) / mq2.Ro, GAS_CO);
  Serial.print("Carbon monoxide:");
  Serial.print(value);
  Serial.print(" ppm\n");
  return value;
}

float get_gasLPG_MQ2() {
  float value = mq2.MQGetGasPercentage(MQRead(analogRead(MQ2_ANALOG_PIN)) / mq2.Ro, GAS_LPG);
  Serial.print("LPG:");
  Serial.print(value);
  Serial.print(" ppm\n");
  return value;
}

float get_smoke_MQ2() {
  float value = mq2.MQGetGasPercentage(MQRead(MQ2_ANALOG_PIN) / mq2.Ro, GAS_SMOKE);
  Serial.print("Smoke:");
  Serial.print(value);
  Serial.print(" ppm\n");
  return value;
}

int get_gas_MQ2() {
  // Read input value and map it from 0 to 100
  int value = analogRead(MQ2_ANALOG_PIN);
  byte bySensorVal = map(value, 0, 1023, 0, 100);
  char cMsg[124];

  // Display input value and mapped value
  sprintf(cMsg, "MQ-2 Sensor Value : %d (%d)", value, bySensorVal);

  // Check for high value
  if (bySensorVal > 60) {
    Serial.print(cMsg);
    Serial.println(F(" *** DISTURBANCE IN THE FORCE! ***"));
  } else {
    Serial.println(cMsg);
  }
  return bySensorVal;
}

float get_motion_HCSR501() {
  float val = digitalRead(HCSR501_DIGITAL_PIN);
  if (val == LOW) {
    // if the value read is low, there was no motion
    Serial.println("No motion");
    return 0;
  } else {
    // if the value read was high, there was motion
    Serial.println("Motion Detected!");
    return 1;
  }
}

String to_string(String name, double value) {
  if (name == "temperature") {
    return "\"" + name + "\"" + " : " + String(value) + " ";
  } else {
    return "\"" + name + "\"" + " : " + String(value) + ", ";
  }
}

//http://sandboxelectronics.com/?p=165
/***************************** MQCalibration ****************************************
Input:   mq_pin - analog channel
Output:  Ro of the sensor
Remarks: This function assumes that the sensor is in clean air. It use
         MQResistanceCalculation to calculates the sensor resistance in clean air
         and then divides it with RO_CLEAN_AIR_FACTOR. RO_CLEAN_AIR_FACTOR is about
         10, which differs slightly between different sensors.
************************************************************************************/
float MQCalibration(int mq_pin) {
  int i;
  float val = 0;

  //take multiple samples
  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {
    val += mq2.MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  //calculate the average value
  val = val / CALIBARAION_SAMPLE_TIMES;

  //divided by RO_CLEAN_AIR_FACTOR yields the Ro
  //according to the chart in the datasheet
  val = val / RO_CLEAN_AIR_FACTOR;

  return val;
}

/*****************************  MQRead *********************************************
Input:   mq_pin - analog channel
Output:  Rs of the sensor
Remarks: This function use MQResistanceCalculation to caculate the sensor resistenc (Rs).
         The Rs changes as the sensor is in the different consentration of the target
         gas. The sample times and the time interval between samples could be configured
         by changing the definition of the macros.
************************************************************************************/
float MQRead(int mq_pin) {
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += mq2.MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  rs = rs / READ_SAMPLE_TIMES;

  return rs;
}
