// ═══════════════════════════════════════════════════════════════════════════════
// SensorValidator.cpp — Validate sensor readings for physical plausibility
// ═══════════════════════════════════════════════════════════════════════════════

#include "SensorValidator.hpp"
#include <cmath>

namespace agri {

bool SensorValidator::validate(SensorData &data) {
  errors_.clear();
  bool valid = true;

  if (!isValidTemperature(data.temperature)) {
    errors_.push_back("Invalid temperature: " +
                      std::to_string(data.temperature));
    valid = false;
  }

  if (!isValidHumidity(data.humidity)) {
    errors_.push_back("Invalid humidity: " + std::to_string(data.humidity));
    valid = false;
  }

  if (!isValidSoilMoisture(data.soilMoisture)) {
    errors_.push_back("Invalid soil moisture: " +
                      std::to_string(data.soilMoisture));
    valid = false;
  }

  if (!isValidPh(data.ph)) {
    errors_.push_back("Invalid pH: " + std::to_string(data.ph));
    valid = false;
  }

  if (!isValidLight(data.lightIntensity)) {
    errors_.push_back("Invalid light intensity: " +
                      std::to_string(data.lightIntensity));
    valid = false;
  }

  data.valid = valid;
  return valid;
}

bool SensorValidator::isValidTemperature(double v) const {
  // Physical range: -50°C to 60°C
  return !std::isnan(v) && !std::isinf(v) && v >= -50.0 && v <= 60.0;
}

bool SensorValidator::isValidHumidity(double v) const {
  // Percentage: 0% to 100%
  return !std::isnan(v) && !std::isinf(v) && v >= 0.0 && v <= 100.0;
}

bool SensorValidator::isValidSoilMoisture(double v) const {
  // Percentage: 0% to 100%
  return !std::isnan(v) && !std::isinf(v) && v >= 0.0 && v <= 100.0;
}

bool SensorValidator::isValidPh(double v) const {
  // pH scale: 0 to 14
  return !std::isnan(v) && !std::isinf(v) && v >= 0.0 && v <= 14.0;
}

bool SensorValidator::isValidLight(double v) const {
  // Percentage: 0% to 100%
  return !std::isnan(v) && !std::isinf(v) && v >= 0.0 && v <= 100.0;
}

} // namespace agri
