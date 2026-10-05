// ═══════════════════════════════════════════════════════════════════════════════
// ConfigManager.cpp — Configuration loading (.env, .conf, CLI args)
// ═══════════════════════════════════════════════════════════════════════════════

#include "ConfigManager.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace agri {

bool ConfigManager::loadEnv(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    // Try environment variables as fallback
    const char *apiKey = std::getenv("FIREBASE_API_KEY");
    const char *dbUrl = std::getenv("FIREBASE_DB_URL");
    const char *projId = std::getenv("FIREBASE_PROJECT_ID");
    const char *tinkercadToken = std::getenv("TINKERCAD_BRIDGE_TOKEN");

    if (apiKey)
      values_["FIREBASE_API_KEY"] = apiKey;
    if (dbUrl)
      values_["FIREBASE_DB_URL"] = dbUrl;
    if (projId)
      values_["FIREBASE_PROJECT_ID"] = projId;
    if (tinkercadToken)
      values_["TINKERCAD_BRIDGE_TOKEN"] = tinkercadToken;

    if (apiKey || dbUrl) {
      std::cout << "[Config] Loaded Firebase config from environment variables."
                << std::endl;
      return true;
    }

    std::cerr << "[Config] No .env file found at: " << filepath << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    parseLine(line);
  }

  std::cout << "[Config] Loaded .env from " << filepath << std::endl;
  return true;
}

bool ConfigManager::loadConfig(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "[Config] Cannot open config: " << filepath << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    parseLine(line);
  }

  std::cout << "[Config] Loaded config from " << filepath << std::endl;
  return true;
}

void ConfigManager::parseArgs(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    auto getNext = [&]() -> std::string {
      if (i + 1 < argc)
        return argv[++i];
      return "";
    };

    if (arg == "--mode") {
      values_["app.mode"] = getNext();
    } else if (arg == "--device") {
      values_["serial.device"] = getNext();
    } else if (arg == "--baud") {
      values_["serial.baud"] = getNext();
    } else if (arg == "--port") {
      values_["server.port"] = getNext();
    } else if (arg == "--host") {
      values_["server.host"] = getNext();
    } else if (arg == "--farm-id") {
      values_["farm.id"] = getNext();
    } else if (arg == "--help" || arg == "-h") {
      std::cout
          << "Usage: smart_agriculture [OPTIONS]\n"
          << "\n"
          << "Options:\n"
          << "  --mode <arduino|simulation>   Operating mode (default: "
             "simulation)\n"
          << "  --device <path>               Serial device (default: "
             "/dev/ttyACM0)\n"
          << "  --baud <rate>                 Baud rate (default: 9600)\n"
          << "  --port <port>                 Web server port (default: 8080)\n"
          << "  --host <host>                 Web server host (default: "
             "0.0.0.0)\n"
          << "  --farm-id <id>                Farm identifier (default: "
             "farm_001)\n"
          << "  --help, -h                    Show this help\n"
          << std::endl;
      std::exit(0);
    }
  }
}

// ── Getters
// ────────────────────────────────────────────────────────────────────

std::string ConfigManager::getFirebaseApiKey() const {
  return get("FIREBASE_API_KEY");
}

std::string ConfigManager::getFirebaseDbUrl() const {
  return get("FIREBASE_DB_URL");
}

std::string ConfigManager::getFirebaseProjectId() const {
  return get("FIREBASE_PROJECT_ID");
}

std::string ConfigManager::getSerialDevice() const {
  return get("serial.device", "/dev/ttyACM0");
}

int ConfigManager::getSerialBaud() const {
  std::string val = get("serial.baud", "9600");
  try {
    return std::stoi(val);
  } catch (...) {
    return 9600;
  }
}

std::string ConfigManager::getAppMode() const {
  std::string mode = get("APP_MODE", "");
  if (!mode.empty())
    return mode;
  return get("app.mode", "simulation");
}

int ConfigManager::getServerPort() const {
  std::string val = get("server.port", "8080");
  try {
    return std::stoi(val);
  } catch (...) {
    return 8080;
  }
}

std::string ConfigManager::getServerHost() const {
  return get("server.host", "0.0.0.0");
}

std::string ConfigManager::getFarmId() const {
  return get("farm.id", "farm_001");
}

std::string ConfigManager::getFarmName() const {
  return get("farm.name", "Green Valley Farm");
}

std::string ConfigManager::getFarmLocation() const {
  return get("farm.location", "Bhubaneswar, Odisha");
}

std::string ConfigManager::getFarmCrop() const {
  return get("farm.crop", "Tomato");
}

int ConfigManager::getDashboardRefreshMs() const {
  std::string val = get("dashboard.refresh_ms", "5000");
  try {
    return std::stoi(val);
  } catch (...) {
    return 5000;
  }
}

int ConfigManager::getPipelineReadIntervalMs() const {
  std::string val = get("pipeline.read_interval_ms", "3000");
  try {
    return std::stoi(val);
  } catch (...) {
    return 3000;
  }
}

std::string ConfigManager::getTinkercadBridgeToken() const {
  return get("TINKERCAD_BRIDGE_TOKEN", "");
}

std::string ConfigManager::get(const std::string &key,
                               const std::string &defaultVal) const {
  auto it = values_.find(key);
  if (it != values_.end() && !it->second.empty()) {
    return it->second;
  }
  return defaultVal;
}

void ConfigManager::set(const std::string &key, const std::string &value) {
  values_[key] = value;
}

bool ConfigManager::has(const std::string &key) const {
  return values_.find(key) != values_.end() && !values_.at(key).empty();
}

// ── Private
// ────────────────────────────────────────────────────────────────────

void ConfigManager::parseLine(const std::string &line) {
  // Skip comments and empty lines
  std::string trimmed = trim(line);
  if (trimmed.empty() || trimmed[0] == '#')
    return;

  auto eqPos = trimmed.find('=');
  if (eqPos == std::string::npos)
    return;

  std::string key = trim(trimmed.substr(0, eqPos));
  std::string val = trim(trimmed.substr(eqPos + 1));

  // Remove surrounding quotes from values
  if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') ||
                          (val.front() == '\'' && val.back() == '\''))) {
    val = val.substr(1, val.size() - 2);
  }

  if (!key.empty()) {
    values_[key] = val;
  }
}

std::string ConfigManager::trim(const std::string &s) {
  auto start = s.find_first_not_of(" \t\r\n");
  auto end = s.find_last_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  return s.substr(start, end - start + 1);
}

} // namespace agri
