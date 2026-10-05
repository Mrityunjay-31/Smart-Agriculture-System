// ═══════════════════════════════════════════════════════════════════════════════
// AlertManager.cpp — Alert generation and management
// ═══════════════════════════════════════════════════════════════════════════════

#include "AlertManager.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>


namespace agri {

AlertManager::AlertManager(const ThresholdConfig &config) : config_(config) {}

std::vector<Alert> AlertManager::evaluate(const SensorData &data) {
  std::vector<Alert> newAlerts;

  auto checkSensor = [&](const std::string &sensor, double value,
                         SensorStatus status) {
    if (status == SensorStatus::WARNING || status == SensorStatus::CRITICAL) {
      Alert alert;
      alert.id = generateId();
      alert.sensor = sensor;
      alert.value = value;
      const auto &range = config_.getRange(sensor);
      alert.thresholdMin = range.optimalMin;
      alert.thresholdMax = range.optimalMax;
      alert.severity = statusToString(status);
      alert.message = generateMessage(sensor, value, range.optimalMin,
                                      range.optimalMax, statusToString(status));
      alert.timestamp = SensorData::currentTimestamp();
      alert.acknowledged = false;

      newAlerts.push_back(alert);

      if (callback_) {
        callback_(alert);
      }
    }
  };

  checkSensor("temperature", data.temperature, data.temperatureStatus);
  checkSensor("humidity", data.humidity, data.humidityStatus);
  checkSensor("soil_moisture", data.soilMoisture, data.soilStatus);
  checkSensor("ph", data.ph, data.phStatus);
  checkSensor("light", data.lightIntensity, data.lightStatus);

  // Store alerts
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &alert : newAlerts) {
      alerts_.push_back(alert);
    }
  }

  return newAlerts;
}

std::vector<Alert> AlertManager::getActiveAlerts() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Alert> active;
  for (const auto &a : alerts_) {
    if (!a.acknowledged) {
      active.push_back(a);
    }
  }
  return active;
}

bool AlertManager::acknowledge(const std::string &alertId) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &alert : alerts_) {
    if (alert.id == alertId) {
      alert.acknowledged = true;
      return true;
    }
  }
  return false;
}

std::vector<Alert> AlertManager::getRecent(int count) const {
  std::lock_guard<std::mutex> lock(mutex_);
  int start = std::max(0, static_cast<int>(alerts_.size()) - count);
  return std::vector<Alert>(alerts_.begin() + start, alerts_.end());
}

void AlertManager::clearAll() {
  std::lock_guard<std::mutex> lock(mutex_);
  alerts_.clear();
}

std::string AlertManager::generateMessage(const std::string &sensor,
                                          double value, double min, double max,
                                          const std::string &severity) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1);

  // Sensor display names
  std::string displayName = sensor;
  std::string unit = "";
  std::string action;

  if (sensor == "temperature") {
    displayName = "Temperature";
    unit = "°C";
    action = (value > max) ? "Consider cooling measures."
                           : "Consider heating or protection.";
  } else if (sensor == "humidity") {
    displayName = "Humidity";
    unit = "%";
    action =
        (value > max) ? "Consider ventilation." : "Consider humidification.";
  } else if (sensor == "soil_moisture") {
    displayName = "Soil moisture";
    unit = "%";
    action =
        (value > max) ? "Reduce irrigation." : "Irrigation may be required.";
  } else if (sensor == "ph") {
    displayName = "pH level";
    unit = "";
    action =
        (value > max) ? "Soil may be too alkaline." : "Soil may be too acidic.";
  } else if (sensor == "light") {
    displayName = "Light intensity";
    unit = "%";
    action = (value > max) ? "Consider shade protection."
                           : "Consider supplemental lighting.";
  }

  oss << severity << ": " << displayName << " is " << value << unit
      << ". Recommended range: " << min << unit << " – " << max << unit << ". "
      << action;

  return oss.str();
}

std::string AlertManager::generateId() {
  static std::mt19937 rng(std::random_device{}());
  static std::uniform_int_distribution<uint64_t> dist;

  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch())
                .count();

  std::ostringstream oss;
  oss << "alert_" << ms << "_" << (dist(rng) % 10000);
  return oss.str();
}

} // namespace agri
