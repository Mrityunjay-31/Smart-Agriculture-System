// ═══════════════════════════════════════════════════════════════════════════════
// SensorData.cpp — SensorData / SystemStatus / Alert implementations
// ═══════════════════════════════════════════════════════════════════════════════

#include "SensorData.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace agri {

// ── SensorData
// ─────────────────────────────────────────────────────────────────

std::string SensorData::currentTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << "Z";
  return ss.str();
}

nlohmann::json SensorData::toJson() const {
  return {{"temperature", temperature},
          {"humidity", humidity},
          {"soilMoisture", soilMoisture},
          {"ph", ph},
          {"lightIntensity", lightIntensity},
          {"overallStatus", overallStatus},
          {"temperatureStatus", statusToString(temperatureStatus)},
          {"humidityStatus", statusToString(humidityStatus)},
          {"soilStatus", statusToString(soilStatus)},
          {"phStatus", statusToString(phStatus)},
          {"lightStatus", statusToString(lightStatus)},
          {"timestamp", timestamp}};
}

SensorData SensorData::fromJson(const nlohmann::json &j) {
  SensorData d;
  if (j.contains("temperature"))
    d.temperature = j["temperature"].get<double>();
  if (j.contains("humidity"))
    d.humidity = j["humidity"].get<double>();
  if (j.contains("soilMoisture"))
    d.soilMoisture = j["soilMoisture"].get<double>();
  if (j.contains("ph"))
    d.ph = j["ph"].get<double>();
  if (j.contains("lightIntensity"))
    d.lightIntensity = j["lightIntensity"].get<double>();
  if (j.contains("overallStatus"))
    d.overallStatus = j["overallStatus"].get<std::string>();
  if (j.contains("timestamp"))
    d.timestamp = j["timestamp"].get<std::string>();

  if (j.contains("temperatureStatus"))
    d.temperatureStatus =
        stringToStatus(j["temperatureStatus"].get<std::string>());
  if (j.contains("humidityStatus"))
    d.humidityStatus = stringToStatus(j["humidityStatus"].get<std::string>());
  if (j.contains("soilStatus"))
    d.soilStatus = stringToStatus(j["soilStatus"].get<std::string>());
  if (j.contains("phStatus"))
    d.phStatus = stringToStatus(j["phStatus"].get<std::string>());
  if (j.contains("lightStatus"))
    d.lightStatus = stringToStatus(j["lightStatus"].get<std::string>());

  d.valid = true;
  return d;
}

// ── SystemStatus
// ───────────────────────────────────────────────────────────────

nlohmann::json SystemStatus::toJson() const {
  return {{"arduinoStatus", arduinoStatus},
          {"firebaseStatus", firebaseStatus},
          {"pipelineStatus", pipelineStatus},
          {"lastConnection", lastConnection},
          {"device", device},
          {"mode", mode}};
}

// ── Alert
// ──────────────────────────────────────────────────────────────────────

nlohmann::json Alert::toJson() const {
  return {{"id", id},
          {"sensor", sensor},
          {"value", value},
          {"thresholdMin", thresholdMin},
          {"thresholdMax", thresholdMax},
          {"severity", severity},
          {"message", message},
          {"timestamp", timestamp},
          {"acknowledged", acknowledged}};
}

Alert Alert::fromJson(const nlohmann::json &j) {
  Alert a;
  if (j.contains("id"))
    a.id = j["id"].get<std::string>();
  if (j.contains("sensor"))
    a.sensor = j["sensor"].get<std::string>();
  if (j.contains("value"))
    a.value = j["value"].get<double>();
  if (j.contains("thresholdMin"))
    a.thresholdMin = j["thresholdMin"].get<double>();
  if (j.contains("thresholdMax"))
    a.thresholdMax = j["thresholdMax"].get<double>();
  if (j.contains("severity"))
    a.severity = j["severity"].get<std::string>();
  if (j.contains("message"))
    a.message = j["message"].get<std::string>();
  if (j.contains("timestamp"))
    a.timestamp = j["timestamp"].get<std::string>();
  if (j.contains("acknowledged"))
    a.acknowledged = j["acknowledged"].get<bool>();
  return a;
}

} // namespace agri
