// ═══════════════════════════════════════════════════════════════════════════════
// SensorProcessor.cpp — Threshold analysis and status computation
// ═══════════════════════════════════════════════════════════════════════════════

#include "SensorProcessor.hpp"

namespace agri {

SensorProcessor::SensorProcessor(const ThresholdConfig &config)
    : config_(config) {}

void SensorProcessor::process(SensorData &data) {
  // Evaluate each sensor against configured thresholds
  data.temperatureStatus = config_.evaluate("temperature", data.temperature);
  data.humidityStatus = config_.evaluate("humidity", data.humidity);
  data.soilStatus = config_.evaluate("soil_moisture", data.soilMoisture);
  data.phStatus = config_.evaluate("ph", data.ph);
  data.lightStatus = config_.evaluate("light", data.lightIntensity);

  // Compute overall crop status
  SensorStatus overall = computeOverallStatus(data);
  data.overallStatus = statusToString(overall);

  // Set timestamp if not already set
  if (data.timestamp.empty()) {
    data.timestamp = SensorData::currentTimestamp();
  }

  // Update latest reading (thread-safe)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    latest_ = data;
    hasData_ = true;
  }
}

SensorData SensorProcessor::getLatest() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return latest_;
}

bool SensorProcessor::hasData() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return hasData_;
}

SensorStatus
SensorProcessor::computeOverallStatus(const SensorData &data) const {
  // If any sensor is CRITICAL, overall is CRITICAL
  if (data.temperatureStatus == SensorStatus::CRITICAL ||
      data.humidityStatus == SensorStatus::CRITICAL ||
      data.soilStatus == SensorStatus::CRITICAL ||
      data.phStatus == SensorStatus::CRITICAL ||
      data.lightStatus == SensorStatus::CRITICAL) {
    return SensorStatus::CRITICAL;
  }

  // If any sensor is WARNING, overall is WARNING
  if (data.temperatureStatus == SensorStatus::WARNING ||
      data.humidityStatus == SensorStatus::WARNING ||
      data.soilStatus == SensorStatus::WARNING ||
      data.phStatus == SensorStatus::WARNING ||
      data.lightStatus == SensorStatus::WARNING) {
    return SensorStatus::WARNING;
  }

  return SensorStatus::OPTIMAL;
}

} // namespace agri
