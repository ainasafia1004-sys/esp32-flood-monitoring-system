// ===== BLYNK SETTINGS =====
#define BLYNK_TEMPLATE_ID   "TMPL6zXPPGal0"
#define BLYNK_TEMPLATE_NAME "Flood Calculation Data"
#define BLYNK_AUTH_TOKEN    "ANT_P-LdFKfdYXYRlpFVsAn2i449rKkM"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ===== ADAFRUIT IO SETTINGS =====
#define IO_USERNAME  "Fyaa"
#define IO_KEY       "aio_Sjlm85YNPTjTRveIaWDxNZCZEEKP"

#include "AdafruitIO_WiFi.h"
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, "Safia", "momomomo");

AdafruitIO_Feed *io_rpm            = io.feed("rpm");
AdafruitIO_Feed *io_windSpeedKMH   = io.feed("wind-speed-kmh");
AdafruitIO_Feed *io_windSpeedMPS   = io.feed("wind-speed-mps");
AdafruitIO_Feed *io_temperature    = io.feed("temperature");
AdafruitIO_Feed *io_humidity       = io.feed("humidity");

// ===== TELEGRAM SETTINGS =====
#define BOT_TOKEN "8640907973:AAHMQGd7I694VdossbGMOlBKeiojMEQjwHk" // Replace with your token from @BotFather
#define CHANNEL_ID "-1003858297586"             // Your exact unique channel ID from URL bar

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// ===== FLOOD THRESHOLDS (MALAYSIA CONTEXT) =====
const float MAX_TEMP     = 35.5;  
const float MAX_HUMIDITY = 97.0;  
const float MAX_WIND_KMH = 25.0;  
const float MAX_RPM      = 800.0; 

unsigned long lastAlertTime = 0;
const unsigned long alertCooldown = 300000; // 5 minutes

// ===== DHT11 =====
#include "DHT.h"
#define DHTPIN 18
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== WIFI =====
char ssid[] = "Safia";
char pass[] = "momomomo";

// ===== HALL SENSOR =====
#define HALL_PIN 5
const float FAN_RADIUS_M = 0.04;

volatile unsigned long pulseCount = 0;
const unsigned long interval = 12000; 
BlynkTimer timer;

// ===== INTERRUPT FUNCTION =====
void IRAM_ATTR countPulse() {
  pulseCount++;
}

// ===== TELEGRAM REPORT SENDER (FIXED MARKDOWN) =====
void sendCombinedTelegramReport(bool isEmergency, String alertReason, float alertValue, String alertUnit, float temp, float humid, float windKMH, float rpmVal) {
  if (!isEmergency || (millis() - lastAlertTime >= alertCooldown || lastAlertTime == 0)) {
    
    String message = "";

    if (isEmergency) {
      message += "🚨 *DAILY FLOOD MONITORING REPORT* 🚨\n";
      message += "📍 *Station:* Kampung Baru Monitoring Node\n\n";
      message += "🔴 *SYSTEM STATUS: CRITICAL ALERT*\n";
      message += "⚠️ *Breach Detected:* " + alertReason + " (" + String(alertValue, 1) + " " + alertUnit + ")\n\n";
    } else {
      message += "📊 *DAILY FLOOD MONITORING REPORT* 📊\n";
      message += "📍 *Station:* Kampung Baru Monitoring Node\n\n";
      message += "🟢 *SYSTEM STATUS: NORMAL*\n";
      message += "Today remains steady with normal weather patterns.\n\n";
    }

    message += "*Current Environmental Readings:*\n";
    message += "🌡️ Temperature: *" + String(temp, 1) + " °C*\n";
    message += "💧 Humidity: *" + String(humid, 1) + " %*\n";
    message += "💨 Wind Speed: *" + String(windKMH, 2) + " km/h* (" + String(rpmVal, 0) + " RPM)\n\n";
    
    message += "🔗 *Live Dashboards:* [View Adafruit Dashboard](https://io.adafruit.com/Fyaa/dashboards/)\n\n";
    
    if (isEmergency) {
      message += "📢 *ATTENTION:* Please clear major drainage areas and move valuables to higher ground safely.";
    } else {
      message += "📢 Status is stable. No active flood risks detected.";
    }

    Serial.println("Attempting to send alert to Telegram Channel...");
    if (bot.sendMessage(CHANNEL_ID, message, "Markdown")) {
      Serial.println("Telegram report posted successfully!");
      if (isEmergency) lastAlertTime = millis(); 
    } else {
      Serial.println("Telegram transmission failed. Syntax parsed improperly or blocked.");
    }
  } else {
    Serial.println("Alert event blocked by 5-minute Telegram cooldown.");
  }
}

// ===== MAIN PROCESSING & SEND LOOP =====
void sendData() {
  noInterrupts();
  unsigned long pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  float elapsedTime = interval / 1000.0; 
  float rpm = (pulses / elapsedTime) * 60.0;

  float windSpeedMPS = (2.0 * PI * FAN_RADIUS_M * rpm) / 60.0;
  float windSpeedKMH = windSpeedMPS * 3.6;

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Hardware Error: Failed to read from DHT11! Skipping loop...");
    return;
  }

  // ===== LOCAL SERIAL MONITOR DEBUG =====
  Serial.println("\n===== [LIVE DATA BROADCAST] =====");
  Serial.print("Pulse Count : "); Serial.println(pulses);
  Serial.print("RPM         : "); Serial.println(rpm, 2);
  Serial.print("Wind Speed  : "); Serial.print(windSpeedMPS, 3); Serial.println(" m/s"); 
  Serial.print("Wind Speed  : "); Serial.print(windSpeedKMH, 3); Serial.println(" km/h");
  Serial.print("Temperature : "); Serial.print(temperature, 1); Serial.println(" °C");
  Serial.print("Humidity    : "); Serial.print(humidity, 1); Serial.println(" %");
  Serial.println("==================================");

  // ===== AUTOMATED EMERGENCY THRESHOLD INSPECTION =====
  if (temperature > MAX_TEMP) {
    sendCombinedTelegramReport(true, "Extreme Pre-Storm Heat", temperature, "°C", temperature, humidity, windSpeedKMH, rpm);
  }
  if (humidity > MAX_HUMIDITY) {
    sendCombinedTelegramReport(true, "Torrential Rain Humidity", humidity, "%", temperature, humidity, windSpeedKMH, rpm);
  }
  if (windSpeedKMH > MAX_WIND_KMH) {
    sendCombinedTelegramReport(true, "Severe Storm Wind Speed", windSpeedKMH, "km/h", temperature, humidity, windSpeedKMH, rpm);
  }
  if (rpm > MAX_RPM) {
    sendCombinedTelegramReport(true, "Critical Rotor RPM", rpm, "RPM", temperature, humidity, windSpeedKMH, rpm);
  }

  // ===== BLYNK PLATFORM TRANSMISSION =====
  Blynk.virtualWrite(V0, rpm);
  Blynk.virtualWrite(V1, windSpeedKMH);
  Blynk.virtualWrite(V5, windSpeedMPS);
  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, humidity);

  // ===== ADAFRUIT IO PLATFORM TRANSMISSION =====
  Serial.println("Pushing variables to Adafruit IO clouds...");
  io_rpm->save(rpm);
  io_windSpeedKMH->save(windSpeedKMH);
  io_windSpeedMPS->save(windSpeedMPS);
  io_temperature->save(temperature);
  io_humidity->save(humidity);
}

void setup() {
  Serial.begin(115200);

  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), countPulse, FALLING);
  dht.begin();

  Serial.println("Connecting to Blynk server network...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  Serial.println("Connecting to Adafruit IO server network...");
  io.connect();
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nAdafruit IO Cloud Connected!");

  secured_client.setInsecure();
  timer.setInterval(interval, sendData);

  Serial.println("Flood Calculation Station Operational!");
}

void loop() {
  Blynk.run();
  io.run(); 
  timer.run();
}