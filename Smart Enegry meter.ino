#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "ThingSpeak.h"

// ================= WiFi & ThingSpeak =================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
WiFiClient client;

unsigned long myChannelNumber = 3085222;
const char * myWriteAPIKey = "YOUR_THINGSPEAK_API_KEY";

// ================= ACS712 Current Sensor =================
const int currentPin = 34;
const int mVperAmp = 185;
const float VREF = 3.3;
const int ADC_RES = 4096;

// ================= ZMPT101B Voltage Sensor =================
#define voltagePin 32
float calibration_factor = 433.33;   // adjust using multimeter

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= Relay & Buzzer =================
#define relayPin 25   // Relay module control pin
#define buzzerPin 26  // Buzzer pin
bool overload = false;

// ================= Theft Sensors =================
#define reedPin 27    // Reed switch
#define hallPin 14    // Hall effect sensor
bool theftDetected = false;

// ================= Timer =================
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 20000; // 20 sec

// ================ Startup SMS control =================
unsigned long bootMillis = 0;
bool startupSMSsent = false;
const unsigned long startupDelay = 5000; // 5 seconds

void setup() {
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Energy Monitor");
  delay(1500);
  lcd.clear();

  // Buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // OFF initially

  // Relay
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Assume LOW = ON (depends on relay type)

  // Theft sensors
  pinMode(reedPin, INPUT_PULLUP);
  pinMode(hallPin, INPUT_PULLUP);

  // WiFi
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // GSM SIM800L/900A (connect TX->16, RX->17)
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // record boot time for startup SMS
  bootMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // ---- Theft Detection ----
  if (!theftDetected) {
    if (digitalRead(reedPin) == LOW || digitalRead(hallPin) == LOW) {
      theftDetected = true;
      sendSMS("⚠️ Theft Alert: Unauthorized access detected!");
      Serial.println("⚠️ Theft Alert sent!");

      // Cut power permanently
      digitalWrite(relayPin, HIGH);
      overload = true; // treat like fault for permanent cutoff

      // LCD message
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("⚠ THEFT ALERT ⚠");
      lcd.setCursor(0, 1);
      lcd.print(" Power Cut Off ");

      // Buzzer warning
      digitalWrite(buzzerPin, HIGH);
      delay(2000);
      digitalWrite(buzzerPin, LOW);
    }
  }

  // ---- Send one-time startup SMS after 5 seconds ----
  if (!startupSMSsent && (currentMillis - bootMillis >= startupDelay)) {
    // take 3 averaged measurements
    float startupCurrent = 0, startupVoltage = 0;
    for (int i = 0; i < 3; i++) {
      startupCurrent += readACCurrent();
      startupVoltage += readACVoltage();
    }
    startupCurrent /= 3.0;
    startupVoltage /= 3.0;
    float startupWattage = startupCurrent * startupVoltage;

    char buf[140];
    snprintf(buf, sizeof(buf),
             "Startup: I=%.2f A, V=%.2f V, P=%.1f W",
             startupCurrent, startupVoltage, startupWattage);

    sendSMS(String(buf));
    Serial.println("Startup SMS sent: " + String(buf));

    startupSMSsent = true;
  }

  // ---- Regular Energy Monitoring ----
  if (currentMillis - lastUpdate >= updateInterval && !theftDetected) {
    lastUpdate = currentMillis;

    float current = readACCurrent();
    float voltage = readACVoltage();
    float wattage = current * voltage;

    // Print Serial
    Serial.print("I = "); Serial.print(current, 2);
    Serial.print(" A   V = "); Serial.print(voltage, 2);
    Serial.print(" V   P = "); Serial.print(wattage, 2);
    Serial.println(" W");

    if (!overload) {
      // Normal display
      lcd.setCursor(0, 0);
      lcd.print("I:");
      lcd.print(current, 2);
      lcd.print("A ");
      lcd.setCursor(9, 0);
      lcd.print("V:");
      lcd.print(voltage, 0);

      lcd.setCursor(0, 1);
      lcd.print("P:");
      lcd.print(wattage, 1);
      lcd.print("W   ");
    } else {
      // Overload or theft display
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" !! FAULT !!");
      lcd.setCursor(0, 1);
      lcd.print(" RESET REQUIRED");
    }

    // Relay Control - Permanent OFF
    if (wattage > 200 && !overload) {
      digitalWrite(relayPin, HIGH);   // Turn OFF relay
      overload = true;
      Serial.println("Overload detected! Relay permanently OFF until reset.");
      sendSMS("Alert: Load exceeded 200W. Power cut off permanently. Reset required.");

      // 🔔 Buzzer for 1 second (one-time)
      digitalWrite(buzzerPin, HIGH);
      delay(2000);
      digitalWrite(buzzerPin, LOW);
    }
    // Send to ThingSpeak
    ThingSpeak.setField(1, current);
    ThingSpeak.setField(2, voltage);
    ThingSpeak.setField(3, wattage);
    ThingSpeak.setField(4, overload ? 1 : 0); // overload/theft status
    ThingSpeak.setField(5, theftDetected ? 1 : 0);
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) Serial.println("ThingSpeak update successful!");
    else {
      Serial.print("ThingSpeak update failed, code: ");
      Serial.println(x);
    }
  }
}

// ================= Function: RMS Current =================
float readACCurrent() {
  const int samples = 1000;
  long sumOfSquares = 0;

  for (int i = 0; i < samples; i++) {
    int raw = analogRead(currentPin);
    float voltage = raw * (VREF / ADC_RES);
    float centered = voltage - (VREF / 2.0);
    float current = (centered * 1000.0) / mVperAmp;
    sumOfSquares += current * current;
  }

  float meanSquare = (float)sumOfSquares / samples;
  return sqrt(meanSquare);
}

// ================= Function: RMS Voltage =================
float readACVoltage() {
  long sumOfSquares = 0;
  const int samples = 1000;
  static float offset = VREF / 2.0;

  for (int i = 0; i < samples; i++) {
    int raw = analogRead(voltagePin);
    float voltage = raw * (VREF / ADC_RES);
    offset = (offset * 0.999) + (voltage * 0.001);
    float centered = voltage - offset;
    sumOfSquares += (centered * centered);
  }

  float meanSquare = (float)sumOfSquares / samples;
  float acRMS = sqrt(meanSquare);
  acRMS *= calibration_factor;
  return acRMS;
}

// ================= Function: Send SMS =================
void sendSMS(String message) {
  Serial.println("Sending SMS: " + message);

  Serial2.println("AT+CMGF=1");  // Text mode
  delay(500);
  Serial2.println("AT+CMGS=\"+91XXXXXXXXXX\""); // Replace with admin number
  delay(500);
  Serial2.print(message);
  delay(500);
  Serial2.write(26);  // Ctrl+Z
  delay(5000);        // wait for SMS to actually send
}
