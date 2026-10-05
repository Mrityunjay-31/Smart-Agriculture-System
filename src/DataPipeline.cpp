// ═══════════════════════════════════════════════════════════════════════════════
// DataPipeline.cpp — Sensor data pipeline orchestration
// Smart Agriculture Monitoring System
//
// Runs in a background thread:
//   Serial/Simulator → Parser → Validator → Processor → Firebase → Alerts
// ═══════════════════════════════════════════════════════════════════════════════

#include "DataPipeline.hpp"
#include <chrono>
#include <iostream>

namespace agri {

DataPipeline::DataPipeline(SensorParser &parser, SensorValidator &validator,
                           SensorProcessor &processor, FirebaseClient &firebase,
                           AlertManager &alerts)
    : parser_(parser), validator_(validator), processor_(processor),
      firebase_(firebase), alerts_(alerts) {}

void DataPipeline::startArduinoMode(const std::string &device, int baud,
                                    const std::string &farmId) {
  if (running_) {
    std::cerr << "[Pipeline] Already running." << std::endl;
    return;
  }

  mode_ = "arduino";
  device_ = device;
  farmId_ = farmId;
  running_ = true;

  thread_ = std::thread(&DataPipeline::arduinoLoop, this, device, baud, farmId);
  std::cout << "[Pipeline] Started in ARDUINO mode: " << device << std::endl;
}

void DataPipeline::startSimulationMode(const std::string &farmId) {
  if (running_) {
    std::cerr << "[Pipeline] Already running." << std::endl;
    return;
  }

  mode_ = "simulation";
  device_ = "virtual";
  farmId_ = farmId;
  running_ = true;

  thread_ = std::thread(&DataPipeline::simulationLoop, this, farmId);
  std::cout << "[Pipeline] Started in SIMULATION mode." << std::endl;
}

void DataPipeline::startTinkercadMode(const std::string &farmId) {
  if (running_) {
    std::cerr << "[Pipeline] Already running." << std::endl;
    return;
  }

  mode_ = "tinkercad";
  device_ = "browser_bridge";
  farmId_ = farmId;
  running_ = true;
  lastReadingTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();

  std::cout
      << "[Pipeline] Started in TINKERCAD mode. Waiting for HTTP ingestion..."
      << std::endl;
}

void DataPipeline::stop() {
  running_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
  std::cout << "[Pipeline] Stopped." << std::endl;
}

SystemStatus DataPipeline::getSystemStatus() const {
  SystemStatus status;

  if (mode_ == "tinkercad") {
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    status.arduinoStatus =
        (now - lastReadingTime_ <= 6000) ? "CONNECTED" : "DISCONNECTED";
  } else {
    status.arduinoStatus = arduinoConnected_ ? "CONNECTED" : "DISCONNECTED";
  }

  status.firebaseStatus =
      firebase_.isConnected() ? "CONNECTED" : "DISCONNECTED";
  status.pipelineStatus = running_ ? "HEALTHY" : "IDLE";
  status.lastConnection = SensorData::currentTimestamp();
  status.device = device_;
  status.mode = mode_;
  return status;
}

// ── Arduino Loop
// ───────────────────────────────────────────────────────────────

void DataPipeline::arduinoLoop(const std::string &device, int baud,
                               const std::string &farmId) {
  SerialPort serial(device, baud);

  while (running_) {
    // Attempt connection if not open
    if (!serial.isOpen()) {
      arduinoConnected_ = false;
      std::cout << "[Pipeline] Connecting to " << device << "..." << std::endl;

      if (!serial.open()) {
        std::cerr << "[Pipeline] Cannot open " << device
                  << ". Retrying in 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        continue;
      }
      arduinoConnected_ = true;
    }

    // Read data line
    std::string dataLine = serial.readLine(readIntervalMs_ + 2000);

    if (dataLine.empty()) {
      // Possible disconnect or timeout
      std::cerr << "[Pipeline] No data received. Checking connection..."
                << std::endl;
      // Try reopening
      serial.close();
      arduinoConnected_ = false;
      std::this_thread::sleep_for(std::chrono::seconds(2));
      continue;
    }

    // Skip non-data lines (e.g., "Smart Agriculture System Initialized...")
    if (dataLine.find("Temp:") == std::string::npos) {
      // Could be the status line from a previous reading or initialization
      // message
      continue;
    }

    // Read the status line that follows
    std::string statusLine = serial.readLine(2000);

    // Process the reading
    processReading(dataLine, statusLine, farmId);

    // Wait before next reading
    std::this_thread::sleep_for(std::chrono::milliseconds(readIntervalMs_));
  }

  serial.close();
  arduinoConnected_ = false;
}

// ── Simulation Loop
// ────────────────────────────────────────────────────────────

void DataPipeline::simulationLoop(const std::string &farmId) {
  SensorSimulator simulator;
  arduinoConnected_ = true; // Virtual connection

  std::cout << "[Pipeline] Simulation mode active. Generating sensor data..."
            << std::endl;

  while (running_) {
    std::string dataLine = simulator.generateDataLine();
    std::string statusLine = simulator.generateStatusLine();

    std::cout << "[SIM] " << dataLine << std::endl;
    std::cout << "[SIM] " << statusLine << std::endl;

    processReading(dataLine, statusLine, farmId);

    std::this_thread::sleep_for(std::chrono::milliseconds(readIntervalMs_));
  }

  arduinoConnected_ = false;
}

// ── Process a Reading
// ──────────────────────────────────────────────────────────

void DataPipeline::processReading(const std::string &dataLine,
                                  const std::string &statusLine,
                                  const std::string &farmId) {
  if (mode_ == "tinkercad") {
    lastReadingTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
  }

  // Step 1: Parse
  auto parsed = parser_.parse(dataLine, statusLine);
  if (!parsed) {
    std::cerr << "[Pipeline] Parse error: " << parser_.getLastError()
              << std::endl;
    return;
  }

  // Step 2: Validate
  if (!validator_.validate(*parsed)) {
    std::cerr << "[Pipeline] Validation failed:";
    for (const auto &err : validator_.getErrors()) {
      std::cerr << " " << err;
    }
    std::cerr << std::endl;
    return;
  }

  // Step 3: Process (threshold analysis)
  processor_.process(*parsed);

  std::cout << "[Pipeline] Processed: T=" << parsed->temperature
            << "°C H=" << parsed->humidity << "% S=" << parsed->soilMoisture
            << "% pH=" << parsed->ph << " L=" << parsed->lightIntensity
            << "% → " << parsed->overallStatus << std::endl;

  // Step 4: Generate alerts
  auto newAlerts = alerts_.evaluate(*parsed);
  for (const auto &alert : newAlerts) {
    std::cout << "[ALERT] " << alert.message << std::endl;

    // Push alert to Firebase
    firebase_.pushAlert(farmId, alert);
  }

  // Step 5: Push to Firebase
  firebase_.pushSensorData(farmId, *parsed);
  firebase_.updateLatest(farmId, *parsed);

  // Step 6: Update system status
  firebase_.updateSystemStatus(farmId, getSystemStatus());
}

} // namespace agri
