#include <YunClient.h>
#include <LiquidCrystal.h>
#include <dht.h>
#include <MQ2.h>

#define DHT11_DIGITAL_PIN 6
#define LM35_ANALOG_PIN 1
#define HCSR501_DIGITAL_PIN 7
#define MQ2_ANALOG_PIN 2
#define BILED1_DIGITAL_PIN 8
#define BILED2_DIGITAL_PIN 9
#define BUZZER_DIGITAL_PIN 10

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

boolean air_hazard = false;
String type_hazard_sensor = "none";

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  //start Bridge
  Bridge.begin();

  // Serial use only for debugging porposes
  // Wait for a serial connection before going into loop()
  // Serial.begin(9600);
  // while (!Serial) ;

  lcd.begin(16, 2);

  // Serial.print("Connecting...");
  lcd.print("Connecting...");
  lcd.setCursor(0, 1);
  
  if (client.connect(server_name, 80)) {
    // Serial.println("Done!");
    lcd.print("Done!");
  } else {
    // Serial.println("Fail");
    lcd.print("Fail!");
    // Freeze on fail
    while (1);
  }
  delay(2000);
  lcd.clear();
  lcd.print("Calibrating...");

  // Serial.print("Calibrating...\n");
  mq2.Ro = MQCalibration(MQ2_ANALOG_PIN);
  // Serial.print("Calibration is done.\n");

  pinMode(BILED1_DIGITAL_PIN, OUTPUT);
  pinMode(BILED2_DIGITAL_PIN, OUTPUT);
  pinMode(BUZZER_DIGITAL_PIN, OUTPUT);

  lcd.clear();
  lcd.print("Ready!");
  delay(2000);
  lcd.clear();
  lcd.noDisplay();
}

void loop() {

  packet = create_http_packet();
  process.runShellCommand(packet);

  // DEBUG
  // Serial.print("Ro=");
  // Serial.print(mq2.Ro);
  // Serial.print(" kohm");
  // Serial.print("\n");

  // do nothing until the process finishes,
  // so you get the whole output:
  // while (process.running());
  
  // Read command output. runShellCommand()
  // while (process.available()) {
  //   String result = process.readString();
  //   Serial.println(result);
  // }
  set_color_bidiode();
  
  if (air_hazard) {
    start_buzzer();
  } else {
    lcd.clear();
    lcd.display();
    lcd.print("Everything is ok");
  }
}

void set_color_bidiode() {
  if (air_hazard) {
    digitalWrite(BILED2_DIGITAL_PIN, HIGH);
    digitalWrite(BILED1_DIGITAL_PIN, LOW);
  } else {
    digitalWrite(BILED1_DIGITAL_PIN, HIGH);
    digitalWrite(BILED2_DIGITAL_PIN, LOW);
  }
}

// Check if air hazard exists
// Hazard is considered to be existing until the sensor
// which launched it will not turn off warning
void set_air_hazard(String sensor_type, float current_value, float max_value) {
  if (current_value > max_value) {
    type_hazard_sensor = sensor_type;
    air_hazard = true;
    set_text_display(sensor_type, current_value, true);
  } else {
    if (type_hazard_sensor == sensor_type) {
      type_hazard_sensor = "none";
      air_hazard = false;
      set_text_display(sensor_type, current_value, false);
    }
  }
}

// Disply information about air hazard
void set_text_display(String sensor_name, float value, bool is_hazard) {
  if (is_hazard) {
    lcd.clear();
    lcd.display();
    lcd.setCursor(0, 0);
    lcd.print(sensor_name);
    lcd.setCursor(0, 1);
    if(sensor_name == "Motion") {
      lcd.print("Watch your back!");
    } else {
      lcd.print(value);
    }
  } 
}

String create_http_packet() {
  String payload = get_payload();
  String packet = command + content_type + post_data_command
                  + start_payload + payload + end_payload + adress;
  return packet;
}

String get_payload() {
  String payload = "";

  float carbon = get_carbonmonoxide_MQ9();
  delay(100);
  payload += to_string("carbon monoxide", carbon);

  float gas = get_gasLPG_MQ2();
  delay(100);
  payload += to_string("gas", gas);

  float humidity = get_humidity_DHT11();
  delay(100);
  payload += to_string("humidity", humidity);

  float motion = get_motion_HCSR501();
  delay(100);
  payload += to_string("motion", motion);

  float smoke = get_smoke_MQ2();
  delay(100);
  payload += to_string("smoke", smoke);

  float temperature = get_temperature_LM35();
  delay(100);
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
  set_air_hazard("Humidity", hum, 70);
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
  set_air_hazard("Temperature", temp, 35);
  // Serial.print("TEMPRATURE = ");
  // Serial.print(temp);
  // Serial.print("*C");
  // Serial.println();

  return temp;
}

float get_carbonmonoxide_MQ9() {
  float value = mq2.MQGetGasPercentage(MQRead(analogRead(MQ2_ANALOG_PIN)) / mq2.Ro, GAS_CO);
  Serial.print("Carbon monoxide:");
  Serial.print(value);
  Serial.print(" ppm\n");
  set_air_hazard("Carbon monoxide", value, 60);
  return value;
}

float get_gasLPG_MQ2() {
  float value = mq2.MQGetGasPercentage(MQRead(analogRead(MQ2_ANALOG_PIN)) / mq2.Ro, GAS_LPG);
  Serial.print("LPG:");
  Serial.print(value);
  Serial.print(" ppm\n");
  set_air_hazard("Propan butan", value, 10);
  return value;
}

float get_smoke_MQ2() {
  float value = mq2.MQGetGasPercentage(MQRead(MQ2_ANALOG_PIN) / mq2.Ro, GAS_SMOKE);
  Serial.print("Smoke:");
  Serial.print(value);
  Serial.print(" ppm\n");
  set_air_hazard("Smoke", value, 10);
  return value;
}

int get_gas_MQ2() {
  // Read input value and map it from 0 to 100
  int value = analogRead(MQ2_ANALOG_PIN);
  byte bySensorVal = map(value, 0, 1023, 0, 100);
  char cMsg[124];

  // Display input value and mapped value
  sprintf(cMsg, "MQ-2 Sensor Value : %d (%d)", value, bySensorVal);
  set_air_hazard("Propan butan", value, 60);
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
    set_air_hazard("Motion", 0, 0);
    return 0;
  } else {
    // if the value read was high, there was motion
    Serial.println("Motion Detected!");
    set_air_hazard("Motion", 1, 0);
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

// https://www.arduino.cc/en/Tutorial/melody
// Sends a square wave of the appropriate frequency
// to the piezo, generating the corresponding tone

int length = 1; // the number of notes
char notes[] = "a"; // a space represents a rest
int beats[] = { 2 };
int tempo = 300;


void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };

  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void start_buzzer() {
  for (int i = 0; i < length; i++) {
    if (notes[i] == ' ') {
      delay(beats[i] * tempo); // rest
    } else {
      playNote(notes[i], beats[i] * tempo);
    }
    // pause between notes
    delay(tempo / 2);
  }
}

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(BUZZER_DIGITAL_PIN, HIGH);
    delayMicroseconds(tone);
    digitalWrite(BUZZER_DIGITAL_PIN, LOW);
    delayMicroseconds(tone);
  }
}