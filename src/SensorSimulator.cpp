// ═══════════════════════════════════════════════════════════════════════════════
// SensorSimulator.cpp — Generate realistic sensor data in Arduino format
// Smart Agriculture Monitoring System
//
// Produces output identical to the Arduino sensor node:
//   "Temp: 24.7C | Hum: 42% | Soil: 47% | pH: 7.3 | Light: 100%"
//   "-> STATUS: All crop parameters are optimal."
// ═══════════════════════════════════════════════════════════════════════════════

#include "SensorSimulator.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>


namespace agri {

SensorSimulator::SensorSimulator() : rng_(std::random_device{}()) {}

std::string SensorSimulator::generateReading() {
  std::string dataLine = generateDataLine();
  std::string statusLine = generateStatusLine();
  return dataLine + "\n" + statusLine;
}

std::string SensorSimulator::generateDataLine() {
  // Apply gradual drift to simulate realistic sensor changes
  temp_ = drift(temp_, 5.0, 45.0, 1.5);
  hum_ = drift(hum_, 10.0, 95.0, 3.0);
  soil_ = drift(soil_, 5.0, 90.0, 4.0);
  ph_ = drift(ph_, 4.0, 9.0, 0.3);
  light_ = drift(light_, 5.0, 100.0, 5.0);

  // If critical injection is enabled, occasionally push values outside
  // thresholds
  if (injectCritical_) {
    std::uniform_int_distribution<int> chance(0, 9);
    if (chance(rng_) < 2) { // 20% chance
      // Pick a random sensor to push critical
      std::uniform_int_distribution<int> sensorPick(0, 4);
      switch (sensorPick(rng_)) {
      case 0:
        temp_ = drift(temp_, 35.0, 42.0, 2.0);
        break;
      case 1:
        hum_ = drift(hum_, 5.0, 20.0, 3.0);
        break;
      case 2:
        soil_ = drift(soil_, 3.0, 12.0, 3.0);
        break;
      case 3:
        ph_ = drift(ph_, 3.0, 4.2, 0.3);
        break;
      case 4:
        light_ = drift(light_, 2.0, 10.0, 2.0);
        break;
      }
    }
  }

  // Format in exact Arduino output format
  std::ostringstream oss;
  oss << std::fixed;
  oss << "Temp: " << std::setprecision(1) << temp_ << "C";
  oss << " | Hum: " << std::setprecision(0) << hum_ << "%";
  oss << " | Soil: " << std::setprecision(0) << soil_ << "%";
  oss << " | pH: " << std::setprecision(1) << ph_;
  oss << " | Light: " << std::setprecision(0) << light_ << "%";

  return oss.str();
}

std::string SensorSimulator::generateStatusLine() {
  // Determine if any values are outside optimal ranges
  bool tempOk = (temp_ >= 18.0 && temp_ <= 30.0);
  bool humOk = (hum_ >= 40.0 && hum_ <= 80.0);
  bool soilOk = (soil_ >= 30.0 && soil_ <= 70.0);
  bool phOk = (ph_ >= 5.5 && ph_ <= 7.5);
  bool lightOk = (light_ >= 30.0 && light_ <= 100.0);

  if (tempOk && humOk && soilOk && phOk && lightOk) {
    return "-> STATUS: All crop parameters are optimal.";
  }

  std::string issues;
  if (!tempOk)
    issues += "temperature, ";
  if (!humOk)
    issues += "humidity, ";
  if (!soilOk)
    issues += "soil moisture, ";
  if (!phOk)
    issues += "pH, ";
  if (!lightOk)
    issues += "light, ";

  // Remove trailing ", "
  if (issues.size() >= 2) {
    issues = issues.substr(0, issues.size() - 2);
  }

  return "-> STATUS: Check " + issues + " levels.";
}

double SensorSimulator::drift(double current, double min, double max,
                              double maxDelta) {
  std::uniform_real_distribution<double> dist(-maxDelta, maxDelta);
  double newVal = current + dist(rng_);

  // Clamp to valid range
  newVal = std::max(min, std::min(max, newVal));
  return newVal;
}

} // namespace agri
