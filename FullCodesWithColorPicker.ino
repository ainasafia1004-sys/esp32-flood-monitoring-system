// ===== BLYNK SETTINGS =====
#define BLYNK_TEMPLATE_ID    "TMPL6zXPPGal0"
#define BLYNK_TEMPLATE_NAME "Flood Calculation Data"
#define BLYNK_AUTH_TOKEN     "ANT_P-LdFKfdYXYRlpFVsAn2i449rKkM"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ===== ADAFRUIT IO SETTINGS =====
#define IO_USERNAME  "Fyaa"
#define IO_KEY       "aio_Sjlm85YNPTjTRveIaWDxNZCZEEKP"

#include "AdafruitIO_WiFi.h"
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, "brocky", "ginger2026");

AdafruitIO_Feed *io_rpm            = io.feed("rpm");
AdafruitIO_Feed *io_windSpeedKMH   = io.feed("wind-speed-kmh");
AdafruitIO_Feed *io_windSpeedMPS   = io.feed("wind-speed-mps");
AdafruitIO_Feed *io_temperature    = io.feed("temperature");
AdafruitIO_Feed *io_humidity       = io.feed("humidity");
AdafruitIO_Feed *io_colorStatus    = io.feed("color-status"); // <-- Feed baru untuk Color Picker

// ===== TELEGRAM SETTINGS =====
#define BOT_TOKEN "8640907973:AAHMQGd7I694VdossbGMOlBKeiojMEQjwHk" 
#define CHANNEL_ID "-1003858297586"             

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// ===== FLOOD THRESHOLDS (MALAYSIA CONTEXT) =====
const float MAX_TEMP     = 35.5;  
const float MAX_HUMIDITY = 97.0;  
const float MAX_WIND_KMH = 25.0;  
const float MAX_RPM      = 800.0; 

// ===== BEWARE / WARNING THRESHOLDS (YELLOW STATUS) =====
const float WARN_TEMP     = 33.0;
const float WARN_HUMIDITY = 90.0;
const float WARN_WIND_KMH = 18.0;
const float WARN_RPM      = 600.0;

unsigned long lastAlertTime = 0;
const unsigned long alertCooldown = 300000; 

// ===== DHT11 =====
#include "DHT.h"
#define DHTPIN 18
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== WIFI =====
char ssid[] = "brocky";
char pass[] = "ginger2026";

// ===== HALL SENSOR =====
#define HALL_PIN 5
const float FAN_RADIUS_M = 0.04;

volatile unsigned long pulseCount = 0;
const unsigned long interval = 12000; 

// ===== ESP32-CAM SETTINGS =====
#define CAM_TRIGGER_PIN 27
const unsigned long camInterval = 50000; // 50 seconds

// Global variables to store latest values for camera evaluation
float latestTemp = 0.0;
float latestHumid = 0.0;
float latestWindKMH = 0.0;
float latestRPM = 0.0;

BlynkTimer timer;

// Forward declaration supaya boleh dipanggil dalam fungsi kamera
void sendCombinedTelegramReport(bool isEmergency, String alertReason, float alertValue, String alertUnit, float temp, float humid, float windKMH, float rpmVal);

// ===== INTERRUPT FUNCTION =====
void IRAM_ATTR countPulse() {
  pulseCount++;
}

// ===== CAMERA PROCESSOR & SERIAL VISUALIZER =====
void triggerCameraAndAnalyze() {
  Serial.println("\n--- [CAMERA EVENT START] ---");
  Serial.println("📸 CAMERA STATUS: [ON]");
  Serial.println("📸 Capturing image from Station...");
  
  // Hardware pulse to physical ESP32-CAM if connected
  digitalWrite(CAM_TRIGGER_PIN, HIGH);
  delay(500); 
  digitalWrite(CAM_TRIGGER_PIN, LOW);
  
  Serial.println("📸 Image capture complete. Processing vision analytics...");
  delay(1000); // Simulate processing time

  // Logik penentuan warna indicator & hantar ke Adafruit
  String statusColor = "GREEN";
  String hexColor = "#00FF00"; // Default Hijau (Safe)
  String alertReason = "";
  float alertValue = 0.0;
  String alertUnit = "";

  // 1. Check CRITICAL (RED) conditions first
  if (latestTemp > MAX_TEMP) { statusColor = "RED"; hexColor = "#FF0000"; alertReason = "Extreme Pre-Storm Heat"; alertValue = latestTemp; alertUnit = "°C"; }
  else if (latestHumid > MAX_HUMIDITY) { statusColor = "RED"; hexColor = "#FF0000"; alertReason = "Torrential Rain Humidity"; alertValue = latestHumid; alertUnit = "%"; }
  else if (latestWindKMH > MAX_WIND_KMH) { statusColor = "RED"; hexColor = "#FF0000"; alertReason = "Severe Storm Wind Speed"; alertValue = latestWindKMH; alertUnit = "km/h"; }
  else if (latestRPM > MAX_RPM) { statusColor = "RED"; hexColor = "#FF0000"; alertReason = "Critical Rotor RPM"; alertValue = latestRPM; alertUnit = "RPM"; }
  
  // 2. Check WARNING (YELLOW) conditions next
  else if (latestTemp > WARN_TEMP || latestHumid > WARN_HUMIDITY || latestWindKMH > WARN_WIND_KMH || latestRPM > WARN_RPM) {
    statusColor = "YELLOW";
    hexColor = "#FFFF00"; // Kuning (Beware)
  }

  // Hantar kod warna terus ke Adafruit IO Color Picker
  io_colorStatus->save(hexColor);
  Serial.print("🎨 Adafruit IO Updated: Sent Hex "); Serial.println(hexColor);

  // ===== SERIAL MONITOR & TELEGRAM TRIGGER =====
  Serial.println("----------------------------------------");
  if (statusColor == "RED") {
    Serial.println("🔴🔴🔴 [CAMERA CAPTURED VISUAL STATUS] 🔴🔴🔴");
    Serial.println("🚨 STATUS   : RED - DANGER");
    Serial.println("⚠️ ACTION   : Critical environmental breach! Activating emergency response.");
    
    // TRIGGER TELEGRAM ALERT SEBAB STATUS MERAH / BAHAYA
    sendCombinedTelegramReport(true, alertReason, alertValue, alertUnit, latestTemp, latestHumid, latestWindKMH, latestRPM);
  } 
  else if (statusColor == "YELLOW") {
    Serial.println("💛💛💛 [CAMERA CAPTURED VISUAL STATUS] 💛💛💛");
    Serial.println("⚠️ STATUS   : YELLOW - BEWARE");
    Serial.println("📢 ACTION   : Weather conditions deteriorating. Stay alert.");
  } 
  else {
    Serial.println("💚💚💚 [CAMERA CAPTURED VISUAL STATUS] 💚💚💚");
    Serial.println("✅ STATUS   : GREEN - SAFE");
    Serial.println("👍 ACTION   : Conditions nominal. Normal monitoring routine.");
  }
  Serial.println("----------------------------------------");

  Serial.println("📸 CAMERA STATUS: [OFF]");
  Serial.println("--- [CAMERA EVENT END] ---\n");
}

// ===== TELEGRAM REPORT SENDER =====
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
    
    message += "🔗 *Live Dashboards:* [View Community Climate Monitor](https://io.adafruit.com/Fyaa/dashboards/community-climate-monitor)\n\n";
    
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
      Serial.println("Telegram transmission failed.");
    }
  }
}

// ===== MAIN PROCESSING & SEND LOOP =====
void sendData() {
  noInterrupts();
  unsigned long pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  float elapsedTime = interval / 1000.0; 
  latestRPM = (pulses / elapsedTime) * 60.0;

  float windSpeedMPS = (2.0 * PI * FAN_RADIUS_M * latestRPM) / 60.0;
  latestWindKMH = windSpeedMPS * 3.6;

  latestTemp = dht.readTemperature();
  latestHumid = dht.readHumidity();

  if (isnan(latestTemp) || isnan(latestHumid)) {
    Serial.println("Hardware Error: Failed to read from DHT11! Skipping loop...");
    return;
  }

  // ===== LOCAL SERIAL MONITOR DEBUG =====
  Serial.println("\n===== [LIVE DATA BROADCAST] =====");
  Serial.print("Pulse Count : "); Serial.println(pulses);
  Serial.print("RPM         : "); Serial.println(latestRPM, 2);
  Serial.print("Wind Speed  : "); Serial.print(latestWindKMH, 3); Serial.println(" km/h");
  Serial.print("Temperature : "); Serial.print(latestTemp, 1); Serial.println(" °C");
  Serial.print("Humidity    : "); Serial.print(latestHumid, 1); Serial.println(" %");
  Serial.println("==================================");

  // Automated emergency inspection (Red Alert triggers dari sensor)
  if (latestTemp > MAX_TEMP) sendCombinedTelegramReport(true, "Extreme Pre-Storm Heat", latestTemp, "°C", latestTemp, latestHumid, latestWindKMH, latestRPM);
  if (latestHumid > MAX_HUMIDITY) sendCombinedTelegramReport(true, "Torrential Rain Humidity", latestHumid, "%", latestTemp, latestHumid, latestWindKMH, latestRPM);
  if (latestWindKMH > MAX_WIND_KMH) sendCombinedTelegramReport(true, "Severe Storm Wind Speed", latestWindKMH, "km/h", latestTemp, latestHumid, latestWindKMH, latestRPM);
  if (latestRPM > MAX_RPM) sendCombinedTelegramReport(true, "Critical Rotor RPM", latestRPM, "RPM", latestTemp, latestHumid, latestWindKMH, latestRPM);

  // Cloud Transmissions
  Blynk.virtualWrite(V0, latestRPM);
  Blynk.virtualWrite(V1, latestWindKMH);
  Blynk.virtualWrite(V5, windSpeedMPS);
  Blynk.virtualWrite(V2, latestTemp);
  Blynk.virtualWrite(V3, latestHumid);

  io_rpm->save(latestRPM);
  io_windSpeedKMH->save(latestWindKMH);
  io_windSpeedMPS->save(windSpeedMPS);
  io_temperature->save(latestTemp);
  io_humidity->save(latestHumid);
}

void setup() {
  Serial.begin(115200);

  pinMode(CAM_TRIGGER_PIN, OUTPUT);
  digitalWrite(CAM_TRIGGER_PIN, LOW);

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
  
  // Timers
  timer.setInterval(interval, sendData);                   // Sensor routine (12 seconds)
  timer.setInterval(camInterval, triggerCameraAndAnalyze); // Camera check routine (50 seconds)

  Serial.println("Flood Calculation Station Operational!");
}

void loop() {
  Blynk.run();
  io.run(); 
  timer.run();
}