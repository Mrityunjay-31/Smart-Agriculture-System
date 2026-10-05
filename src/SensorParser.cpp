// ═══════════════════════════════════════════════════════════════════════════════
// SensorParser.cpp — Parse Arduino serial output format
// Smart Agriculture Monitoring System
//
// Parses lines like:
//   "Temp: 24.7C | Hum: 42% | Soil: 47% | pH: 7.3 | Light: 100%"
//   "-> STATUS: All crop parameters are optimal."
// ═══════════════════════════════════════════════════════════════════════════════

#include "SensorParser.hpp"
#include <iostream>
#include <regex>
#include <sstream>


namespace agri {

std::optional<SensorData> SensorParser::parse(const std::string &dataLine,
                                              const std::string &statusLine) {
  auto data = parseDataLine(dataLine);
  if (!data)
    return std::nullopt;

  if (!statusLine.empty()) {
    data->overallStatus = parseStatusLine(statusLine);
  }

  data->timestamp = SensorData::currentTimestamp();
  return data;
}

std::optional<SensorData> SensorParser::parseDataLine(const std::string &line) {
  if (line.empty()) {
    lastError_ = "Empty data line";
    return std::nullopt;
  }

  // Split by " | " to get individual sensor segments
  std::vector<std::string> segments;
  std::string remaining = line;
  size_t pos;

  while ((pos = remaining.find(" | ")) != std::string::npos) {
    segments.push_back(remaining.substr(0, pos));
    remaining = remaining.substr(pos + 3);
  }
  segments.push_back(remaining);

  if (segments.size() < 5) {
    lastError_ =
        "Expected 5 sensor segments, got " + std::to_string(segments.size());
    return std::nullopt;
  }

  SensorData data;

  // Extract each value: "Temp: 24.7C", "Hum: 42%", etc.
  auto temp = extractValue(segments[0], "Temp:");
  auto hum = extractValue(segments[1], "Hum:");
  auto soil = extractValue(segments[2], "Soil:");
  auto ph = extractValue(segments[3], "pH:");
  auto light = extractValue(segments[4], "Light:");

  if (!temp || !hum || !soil || !ph || !light) {
    lastError_ = "Failed to extract numeric values from data line";
    return std::nullopt;
  }

  data.temperature = *temp;
  data.humidity = *hum;
  data.soilMoisture = *soil;
  data.ph = *ph;
  data.lightIntensity = *light;

  return data;
}

std::string SensorParser::parseStatusLine(const std::string &line) {
  // Expected format: "-> STATUS: All crop parameters are optimal."
  static const std::regex statusRegex(R"(->\s*STATUS:\s*(.+))");

  std::smatch match;
  if (std::regex_search(line, match, statusRegex) && match.size() > 1) {
    std::string status = match[1].str();
    // Trim trailing period and whitespace
    while (!status.empty() && (status.back() == '.' || status.back() == ' ')) {
      status.pop_back();
    }
    return status;
  }

  return line; // Return as-is if no pattern match
}

std::optional<double> SensorParser::extractValue(const std::string &segment,
                                                 const std::string &label) {
  // Find the label position
  size_t labelPos = segment.find(label);
  if (labelPos == std::string::npos) {
    return std::nullopt;
  }

  // Extract the portion after the label
  std::string after = segment.substr(labelPos + label.size());

  // Use regex to extract the numeric value
  // Matches: optional whitespace, optional sign, digits, optional decimal
  static const std::regex numRegex(R"(\s*(-?\d+\.?\d*))");

  std::smatch match;
  if (std::regex_search(after, match, numRegex) && match.size() > 1) {
    try {
      return std::stod(match[1].str());
    } catch (const std::exception &) {
      return std::nullopt;
    }
  }

  return std::nullopt;
}

} // namespace agri
