// ═══════════════════════════════════════════════════════════════════════════════
// ThresholdConfig.cpp — Load and evaluate sensor thresholds
// ═══════════════════════════════════════════════════════════════════════════════

#include "ThresholdConfig.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>


namespace agri {

ThresholdRange ThresholdConfig::defaultRange_{0.0, 100.0, 0.0, 100.0};

bool ThresholdConfig::load(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "[ThresholdConfig] Cannot open: " << filepath << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#')
      continue;
    parseLine(line);
  }

  loaded_ = true;
  std::cout << "[ThresholdConfig] Loaded " << thresholds_.size()
            << " sensor thresholds from " << filepath << std::endl;
  return true;
}

SensorStatus ThresholdConfig::evaluate(const std::string &sensor,
                                       double value) const {
  const auto &range = getRange(sensor);

  // Check optimal range first
  if (value >= range.optimalMin && value <= range.optimalMax) {
    return SensorStatus::OPTIMAL;
  }

  // Check warning range
  if (value >= range.warningMin && value <= range.warningMax) {
    return SensorStatus::WARNING;
  }

  // Outside warning range → CRITICAL
  return SensorStatus::CRITICAL;
}

const ThresholdRange &
ThresholdConfig::getRange(const std::string &sensor) const {
  auto it = thresholds_.find(sensor);
  if (it != thresholds_.end()) {
    return it->second;
  }
  return defaultRange_;
}

void ThresholdConfig::parseLine(const std::string &line) {
  // Format: "sensor.parameter = value"
  // Example: "temperature.optimal_min = 18.0"

  auto eqPos = line.find('=');
  if (eqPos == std::string::npos)
    return;

  std::string key = line.substr(0, eqPos);
  std::string val = line.substr(eqPos + 1);

  // Trim whitespace
  auto trim = [](std::string &s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
  };
  trim(key);
  trim(val);

  // Split key into sensor name and parameter
  auto dotPos = key.find('.');
  if (dotPos == std::string::npos)
    return;

  std::string sensor = key.substr(0, dotPos);
  std::string param = key.substr(dotPos + 1);

  // Normalize sensor names
  // "soil_moisture" in config maps to "soil_moisture" internally
  // The processor will use these names when calling evaluate()

  double numVal;
  try {
    numVal = std::stod(val);
  } catch (...) {
    std::cerr << "[ThresholdConfig] Invalid value for " << key << ": " << val
              << std::endl;
    return;
  }

  auto &range = thresholds_[sensor];

  if (param == "optimal_min")
    range.optimalMin = numVal;
  else if (param == "optimal_max")
    range.optimalMax = numVal;
  else if (param == "warning_min")
    range.warningMin = numVal;
  else if (param == "warning_max")
    range.warningMax = numVal;
}

} // namespace agri
