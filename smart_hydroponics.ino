// ============================================================
//  Smart IoT Enabled Hydroponic Farming
//  Vidyalankar Institute of Technology, Mumbai
//  Department: Electronics & Telecommunication Engineering
//
//  Hardware  : ESP8266 NodeMCU
//  Cloud     : AWS IoT Core (eu-north-1) — MQTT over TLS port 8883
//  Sensors   : DS18B20, DHT11, pH, TDS, LDR, Water Level (via 74HC4067 MUX)
//  Actuators : Water Pump (D8), LED Grow Light (D7) via relay module
//
//  MQTT Topics:
//    Publish  → sensor/data      (JSON sensor readings every 5s)
//    Subscribe← device/control   (commands: pump_on/off, light_on/off)
//
//  Libraries required (install via Arduino IDE Library Manager):
//    - PubSubClient       by Nick O'Leary
//    - DallasTemperature  by Miles Burton
//    - OneWire            by Paul Stoffregen
//    - DHT sensor library by Adafruit
//
//  Board: ESP8266 NodeMCU 1.0 (ESP-12E Module)
//  See README.md for wiring, calibration, and AWS setup guide.
// ============================================================

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <time.h>

// ── WiFi credentials ─────────────────────────────────────────
#define WIFI_SSID     "YOUR_WIFI_SSID"       // <-- replace
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"   // <-- replace

// ── AWS IoT Core ─────────────────────────────────────────────
#define AWS_IOT_ENDPOINT  "YOUR_ENDPOINT.iot.REGION.amazonaws.com"  // <-- replace
#define CLIENT_ID         "ESP8266_Device"
#define AWS_IOT_TOPIC     "sensor/data"
#define AWS_CONTROL_TOPIC "device/control"

// ── TLS Certificates ─────────────────────────────────────────
// Paste your certificate contents below.
// AWS Root CA 1: https://www.amazontrust.com/repository/AmazonRootCA1.pem
static const char CA_CERT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<PASTE AWS ROOT CA 1 HERE>
-----END CERTIFICATE-----
)EOF";

// Your device certificate (.pem.crt from AWS IoT console)
static const char CERTIFICATE[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<PASTE YOUR DEVICE CERTIFICATE HERE>
-----END CERTIFICATE-----
)EOF";

// Your private key (.pem.key from AWS IoT console)
static const char PRIVATE_KEY[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
<PASTE YOUR PRIVATE KEY HERE>
-----END RSA PRIVATE KEY-----
)EOF";

// ── Pin Definitions ───────────────────────────────────────────
#define DS18B20_PIN     D4    // Water temperature (OneWire)
#define DHT_PIN         D5    // Air temperature & humidity
#define DHT_TYPE        DHT11
#define MUX_SIG         A0    // 74HC4067 Mux analog output
#define S0              D0    // Mux select bit 0
#define S1              D1    // Mux select bit 1
#define S2              D2    // Mux select bit 2
#define S3              D3    // Mux select bit 3
#define GROW_LIGHT_PIN  D7    // Relay → LED grow light
#define WATER_PUMP_PIN  D8    // Relay → water pump

// ── Mux Channel Assignments ───────────────────────────────────
#define PH_CHANNEL          1
#define TDS_CHANNEL         0
#define LDR_CHANNEL         2
#define WATER_LEVEL_CHANNEL 3

// ── pH Calibration Constants ──────────────────────────────────
// See calibration guide at bottom of this file.
// These defaults give pH values in the 5.5–7.0 range.
#define PH_SLOPE      -5.70f
#define PH_INTERCEPT  21.34f

// ── Publish Interval ──────────────────────────────────────────
#define PUBLISH_INTERVAL_MS 5000UL   // publish every 5 seconds

// ── Objects ───────────────────────────────────────────────────
BearSSL::WiFiClientSecure wifiClient;
BearSSL::X509List   caCert(CA_CERT);
BearSSL::X509List   clientCert(CERTIFICATE);
BearSSL::PrivateKey privateKey(PRIVATE_KEY);

PubSubClient mqttClient(wifiClient);

OneWire           oneWire(DS18B20_PIN);
DallasTemperature waterTempSensor(&oneWire);
DHT               dht(DHT_PIN, DHT_TYPE);

unsigned long lastPublishTime = 0;

// ============================================================
//  MUX HELPER — select analog input channel on 74HC4067
// ============================================================
void setMuxChannel(int ch) {
  digitalWrite(S0,  ch        & 0x01);
  digitalWrite(S1, (ch >> 1)  & 0x01);
  digitalWrite(S2, (ch >> 2)  & 0x01);
  digitalWrite(S3, (ch >> 3)  & 0x01);
  delay(10);  // allow signal to settle
}

// ============================================================
//  SENSOR READ FUNCTIONS
// ============================================================

/**
 * readPH()
 * Converts analog voltage to pH using linear calibration.
 * Calibrate PH_SLOPE and PH_INTERCEPT with pH 4.0 & 7.0 buffers.
 * Returns pH clamped to 0.0–14.0.
 */
float readPH() {
  setMuxChannel(PH_CHANNEL);
  float voltage = analogRead(MUX_SIG) * (3.3f / 1023.0f);
  float ph = PH_SLOPE * voltage + PH_INTERCEPT;
  ph = constrain(ph, 0.0f, 14.0f);
  return ph;
}

/**
 * readTDS()
 * Converts analog voltage to TDS (ppm) using polynomial formula.
 * Calibrated for 3.3V reference. Typical output: 600–1200 ppm.
 */
float readTDS() {
  setMuxChannel(TDS_CHANNEL);
  float voltage = analogRead(MUX_SIG) * (3.3f / 1023.0f);
  float tds = (133.42f * voltage * voltage * voltage
             - 255.86f * voltage * voltage
             + 857.39f * voltage) * 0.5f;
  return tds;
}

/**
 * readLDR()
 * Returns light intensity estimate in lux (0–1000 range).
 */
float readLDR() {
  setMuxChannel(LDR_CHANNEL);
  return (analogRead(MUX_SIG) / 1023.0f) * 1000.0f;
}

/**
 * readWaterLevel()
 * Returns water level as a percentage (0–100%).
 */
float readWaterLevel() {
  setMuxChannel(WATER_LEVEL_CHANNEL);
  return (analogRead(MUX_SIG) / 1023.0f) * 100.0f;
}

// ============================================================
//  PUBLISH SENSOR DATA TO AWS IoT (topic: sensor/data)
// ============================================================
void sendSensorData() {
  // Water temperature via DS18B20
  waterTempSensor.requestTemperatures();
  float waterTemp = waterTempSensor.getTempCByIndex(0);

  // Air temperature & humidity via DHT11
  float airTemp  = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(airTemp) || isnan(humidity)) {
    Serial.println("[WARN] DHT11 read failed — skipping this cycle");
    return;
  }

  // Analog sensors via MUX
  float phValue    = readPH();
  float tdsValue   = readTDS();
  float ldrValue   = readLDR();
  float waterLevel = readWaterLevel();

  // Build JSON payload
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{"
      "\"water_temp\":%.2f,"
      "\"air_temp\":%.2f,"
      "\"humidity\":%.2f,"
      "\"pH\":%.2f,"
      "\"TDS\":%.1f,"
      "\"LDR\":%.1f,"
      "\"water_level\":%.1f"
    "}",
    waterTemp, airTemp, humidity,
    phValue, tdsValue, ldrValue, waterLevel
  );

  if (mqttClient.publish(AWS_IOT_TOPIC, payload)) {
    Serial.print("[MQTT] Published → ");
    Serial.println(payload);
  } else {
    Serial.println("[MQTT] Publish FAILED");
  }
}

// ============================================================
//  MQTT CALLBACK — handles incoming commands
//  Topic   : device/control
//  Commands: pump_on | pump_off | light_on | light_off
// ============================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String cmd;
  for (unsigned int i = 0; i < length; i++) cmd += (char)payload[i];
  cmd.trim();

  Serial.print("[MQTT] Command received: ");
  Serial.println(cmd);

  if      (cmd == "pump_on")   { digitalWrite(WATER_PUMP_PIN, HIGH); Serial.println("→ Pump ON");        }
  else if (cmd == "pump_off")  { digitalWrite(WATER_PUMP_PIN, LOW);  Serial.println("→ Pump OFF");       }
  else if (cmd == "light_on")  { digitalWrite(GROW_LIGHT_PIN, HIGH); Serial.println("→ Grow Light ON");  }
  else if (cmd == "light_off") { digitalWrite(GROW_LIGHT_PIN, LOW);  Serial.println("→ Grow Light OFF"); }
  else                         { Serial.println("→ Unknown command — ignored");                           }
}

// ============================================================
//  MQTT RECONNECT — retries until connected
// ============================================================
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting to AWS IoT...");
    if (mqttClient.connect(CLIENT_ID)) {
      Serial.println(" connected!");
      mqttClient.subscribe(AWS_CONTROL_TOPIC);
      Serial.print("[MQTT] Subscribed to: ");
      Serial.println(AWS_CONTROL_TOPIC);
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" — retrying in 2s");
      delay(2000);
    }
  }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Smart Hydroponics System Booting ===");

  // Configure output pins
  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT); pinMode(S3, OUTPUT);
  pinMode(GROW_LIGHT_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  digitalWrite(GROW_LIGHT_PIN, LOW);  // default OFF
  digitalWrite(WATER_PUMP_PIN, LOW);  // default OFF

  // Initialise sensors
  waterTempSensor.begin();
  dht.begin();

  // Connect to WiFi
  Serial.print("[WiFi] Connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());

  // Sync time via NTP — mandatory for TLS certificate validation
  Serial.print("[NTP]  Syncing time");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 1000000000UL) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[NTP]  Time synced");

  // Load TLS credentials for AWS IoT mutual auth
  wifiClient.setTrustAnchors(&caCert);
  wifiClient.setClientRSACert(&clientCert, &privateKey);

  // Configure MQTT client
  mqttClient.setServer(AWS_IOT_ENDPOINT, 8883);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);  // handles full JSON payload

  Serial.println("[System] Setup complete — entering main loop\n");
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  // Keep MQTT connection alive
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();  // process incoming control commands

  // Publish sensor data at defined interval (non-blocking)
  unsigned long now = millis();
  if (now - lastPublishTime >= PUBLISH_INTERVAL_MS) {
    lastPublishTime = now;
    sendSensorData();
  }
}

// ============================================================
//  pH CALIBRATION GUIDE
//  ─────────────────────────────────────────────────────────
//  1. Open Serial Monitor (115200 baud)
//  2. Dip probe in pH 7.0 buffer solution
//     → note the raw voltage printed: call it V7
//  3. Dip probe in pH 4.0 buffer solution
//     → note the raw voltage printed: call it V4
//  4. Calculate new constants:
//       PH_SLOPE     = (4.0 - 7.0) / (V4 - V7)
//       PH_INTERCEPT = 7.0 - PH_SLOPE * V7
//  5. Update PH_SLOPE and PH_INTERCEPT at top of this file.
// ============================================================
