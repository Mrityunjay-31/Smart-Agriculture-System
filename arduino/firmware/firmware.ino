#include <DHT.h>

// ══════════════════════════════════════════════════════════════════════════
// Smart Agriculture Monitoring System - Sensor Firmware
// ══════════════════════════════════════════════════════════════════════════

// Hardware Pin Definitions
#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // DHT 11 (or DHT 22)
#define SOIL_PIN A0   // Analog pin for Soil Moisture Sensor
#define PH_PIN A1     // Analog pin for pH Sensor
#define LDR_PIN A2    // Analog pin for Light Sensor (LDR)

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Sampling Configuration
const unsigned long SAMPLING_INTERVAL_MS = 5000; // Send data every 5 seconds
unsigned long lastSampleTime = 0;

void setup() {
  Serial.begin(9600);

  // Wait for serial port to connect (required for Native USB boards)
  while (!Serial) {
    ;
  }

  dht.begin();

  // Set analog reference to DEFAULT (5V or 3.3V depending on board)
  analogReference(DEFAULT);

  Serial.println("[ARDUINO] Smart Agriculture Sensor Node Initialized");
}

void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking delay for sampling
  if (currentMillis - lastSampleTime >= SAMPLING_INTERVAL_MS) {
    lastSampleTime = currentMillis;
    readAndSendSensors();
  }
}

void readAndSendSensors() {
  // 1. Read Temperature & Humidity from DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Celsius

  // Check if any reads failed and exit early
  if (isnan(h) || isnan(t)) {
    Serial.println("[ERROR] Failed to read from DHT sensor!");
    // You could decide to return here, or just send default/fallback values
    t = 0.0;
    h = 0.0;
  }

  // 2. Read Soil Moisture (Analog 0-1023 -> maps to percentage)
  // Usually, higher analog value = drier soil (depends on sensor)
  // Assuming 1023 is dry air, 300 is wet water
  int soilRaw = analogRead(SOIL_PIN);
  float soilPercent = map(soilRaw, 1023, 300, 0, 100);
  if (soilPercent < 0)
    soilPercent = 0;
  if (soilPercent > 100)
    soilPercent = 100;

  // 3. Read Light Level (Analog 0-1023 -> maps to percentage)
  // Assuming higher analog reading = more light
  int lightRaw = analogRead(LDR_PIN);
  float lightPercent = map(lightRaw, 0, 1023, 0, 100);

  // 4. Read pH value (Analog 0-1023)
  // Calibration required for precise pH. Example generic mapping for standard
  // 5V probe: Voltages usually map from 0 to 14 pH linearly.
  int phRaw = analogRead(PH_PIN);
  float voltage = phRaw * (5.0 / 1023.0);
  float phValue =
      3.5 * voltage; // Simple approximation (calibrate based on datasheet)

  // 5. Determine Overall Sensor Status
  // A simple onboard classifier. The heavier logic runs on our C++ Linux
  // backend.
  String status = "OPTIMAL";
  if (t > 35.0 || t < 5.0 || soilPercent < 20.0 || phValue < 5.5 ||
      phValue > 8.5) {
    status = "WARNING";
  }

  // 6. Format and send string matching the backend's regex parser exactly
  // Regex format: ^T=([0-9.]+)C H=([0-9.]+)% S=([0-9.]+)% pH=([0-9.]+)
  // L=([0-9.]+)% STATUS=([A-Z]+)
  Serial.print("T=");
  Serial.print(t, 1);
  Serial.print("C H=");
  Serial.print(h, 0); // Humidity is typically integer precision in DHT11
  Serial.print("% S=");
  Serial.print(soilPercent, 0);
  Serial.print("% pH=");
  Serial.print(phValue, 1);
  Serial.print(" L=");
  Serial.print(lightPercent, 0);
  Serial.print("% STATUS=");
  Serial.print(status);

  // Must end with a newline for the POSIX serial reader buffer!
  Serial.println();
}
