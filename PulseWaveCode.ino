#include "ThingSpeak.h"
#include <PulseSensorPlayground.h>
#include <LiquidCrystal_I2C.h>
#include <DFRobot_MLX90614.h>
#include <Wire.h>
#include <WiFi.h>   

//WiFi & ThingSpeak
const char* ssid = "VodafoneMobileWiFi-B53841";   
const char* password = "8387714391"; 
WiFiClient client;

unsigned long myChannelNumber = 3062329;
const char* myWriteAPIKey = "CI3TYTGFXHU3LQS9";
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// Pulse Sensor
const int PulseWire = 34;
int myBPM = 0;
int Threshold = 500;
PulseSensorPlayground pulseSensor;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

//IR Thermometer (MLX90614)
DFRobot_MLX90614_I2C sensor;

void setup() {
  Serial.begin(115200);

  // Setup pulse sensor
  pulseSensor.analogInput(PulseWire);
  pulseSensor.setThreshold(Threshold);
  if (pulseSensor.begin()) {
    Serial.println("Pulse sensor started successfully.");
  }

  // Connect WiFi
  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // LCD setup
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(2,0);
  lcd.print("Health Device");

  // Init temperature sensor
  while (NO_ERR != sensor.begin()) {
    Serial.println("Temp sensor connection failed. Check wiring.");
    delay(3000);
  }
  Serial.println("Temperature sensor OK!");

  sensor.enterSleepMode();
  delay(50);
  sensor.enterSleepMode(false);
  delay(200);
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {

    // Reconnect WiFi if lost
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Reconnecting WiFi...");
      WiFi.begin(ssid, password);
      delay(2000);
    }

    // Check heartbeat
    if (pulseSensor.sawStartOfBeat()) {
      myBPM = pulseSensor.getBeatsPerMinute();
      Serial.println("♥ Heartbeat detected!");
      Serial.print("BPM: ");
      Serial.println(myBPM);
    }

    // Read temperatures
    float ambientTemp = sensor.getAmbientTempCelsius();
    float objectTemp  = sensor.getObjectTempCelsius();

    Serial.print("Ambient Temp: "); Serial.print(ambientTemp); Serial.println(" °C");
    Serial.print("Object Temp: ");  Serial.print(objectTemp);  Serial.println(" °C");

    // Update LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("BPM: ");
    lcd.print(myBPM);
    lcd.setCursor(0,1);
    lcd.print("Temp: ");
    lcd.print(objectTemp);
    lcd.print("C");

    // Send BOTH BPM and Temperature to ThingSpeak
    ThingSpeak.setField(1, myBPM);        // Field 1 = Heart Rate
    ThingSpeak.setField(2, objectTemp);   // Field 2 = Temperature (°C)

    int statusCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (statusCode == 200) {
      Serial.println("ThingSpeak update successful (BPM + Temp).");
    } else {
      Serial.println("ThingSpeak update failed. Code: " + String(statusCode));
    }

    lastTime = millis();
  }
}