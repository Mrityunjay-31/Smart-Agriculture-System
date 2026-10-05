// ═══════════════════════════════════════════════════════════════════════════════
// main.cpp — Smart Agriculture Monitoring System Entry Point
// Smart Agriculture Monitoring System
//
// Usage:
//   ./smart_agriculture --mode simulation --port 8080
//   ./smart_agriculture --mode arduino --device /dev/ttyACM0 --baud 9600
//   ./smart_agriculture --help
//
// Architecture:
//   1. Load configuration (.env, config.conf, CLI args)
//   2. Initialize components (ThresholdConfig, FirebaseClient, AuthManager,
//   etc.)
//   3. Start data pipeline in background thread (Arduino or Simulation mode)
//   4. Start Crow web server on main thread
//   5. Handle SIGINT/SIGTERM for graceful shutdown
// ═══════════════════════════════════════════════════════════════════════════════

#include "AlertManager.hpp"
#include "AuthManager.hpp"
#include "ConfigManager.hpp"
#include "DataPipeline.hpp"
#include "FirebaseClient.hpp"
#include "SensorParser.hpp"
#include "SensorProcessor.hpp"
#include "SensorValidator.hpp"
#include "SessionManager.hpp"
#include "ThresholdConfig.hpp"
#include "WebServer.hpp"

using namespace agri;

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>

// Global shutdown flag
static std::atomic<bool> g_shutdown{false};

void signalHandler(int signum) {
  std::cout << "\n[Main] Received signal " << signum << ". Shutting down..."
            << std::endl;
  g_shutdown = true;
}

int main(int argc, char *argv[]) {
  // ── Banner ─────────────────────────────────────────────────────────────
  std::cout << R"(
  ╔═══════════════════════════════════════════════════════════╗
  ║       🌱 Smart Agriculture Monitoring System 🌱           ║
  ║                                                           ║
  ║   Monitor → Analyze → Grow                                ║
  ╚═══════════════════════════════════════════════════════════╝
    )" << std::endl;

  // ── Signal handlers for graceful shutdown ──────────────────────────────
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  // ── Step 1: Load Configuration ─────────────────────────────────────────
  ConfigManager config;
  config.loadEnv(".env");
  config.loadConfig("config/config.conf");
  config.parseArgs(argc, argv);

  std::string mode = config.getAppMode();
  std::string device = config.getSerialDevice();
  int baud = config.getSerialBaud();
  int port = config.getServerPort();
  std::string host = config.getServerHost();
  std::string farmId = config.getFarmId();

  std::cout << "[Config] Mode:   " << mode << std::endl;
  std::cout << "[Config] Device: " << device << std::endl;
  std::cout << "[Config] Baud:   " << baud << std::endl;
  std::cout << "[Config] Port:   " << port << std::endl;
  std::cout << "[Config] Farm:   " << farmId << std::endl;

  // ── Step 2: Load Threshold Configuration ───────────────────────────────
  ThresholdConfig thresholds;
  if (!thresholds.load("config/thresholds.conf")) {
    std::cerr << "[Main] WARNING: Using default thresholds." << std::endl;
  }

  // ── Step 3: Initialize Firebase ────────────────────────────────────────
  std::string fbDbUrl = config.getFirebaseDbUrl();
  std::string fbApiKey = config.getFirebaseApiKey();

  if (fbDbUrl.empty() || fbApiKey.empty()) {
    std::cerr << "[Main] WARNING: Firebase not configured. "
              << "Data will not be persisted." << std::endl;
    std::cerr << "       Set FIREBASE_DB_URL and FIREBASE_API_KEY in .env"
              << std::endl;
  }

  FirebaseClient firebase(fbDbUrl, fbApiKey);
  firebase.init();

  // ── Step 4: Initialize Components ──────────────────────────────────────
  AuthManager auth(fbApiKey);
  SensorParser parser;
  SensorValidator validator;
  SensorProcessor processor(thresholds);
  AlertManager alerts(thresholds);
  SessionManager sessions;

  // ── Step 5: Start Data Pipeline ────────────────────────────────────────
  DataPipeline pipeline(parser, validator, processor, firebase, alerts);
  pipeline.setReadInterval(config.getPipelineReadIntervalMs());

  if (mode == "arduino") {
    std::cout << "[Main] Starting ARDUINO mode on " << device << std::endl;
    pipeline.startArduinoMode(device, baud, farmId);
  } else if (mode == "tinkercad") {
    std::cout << "[Main] Starting TINKERCAD mode" << std::endl;
    pipeline.startTinkercadMode(farmId);
  } else {
    std::cout << "[Main] Starting SIMULATION mode" << std::endl;
    pipeline.startSimulationMode(farmId);
  }

  // Store initial farm info
  firebase.storeFarmInfo(farmId, {{"name", config.getFarmName()},
                                  {"location", config.getFarmLocation()},
                                  {"cropType", config.getFarmCrop()}});

  // ── Step 6: Start Web Server ───────────────────────────────────────────
  WebServer server(pipeline, processor, firebase, auth, alerts, sessions,
                   config);

  // Web server runs on main thread (Crow's run() is blocking)
  // The pipeline runs in its own background thread
  server.start(port, host);

  // ── Cleanup ────────────────────────────────────────────────────────────
  pipeline.stop();

  std::cout << "[Main] Goodbye! 🌿" << std::endl;
  return 0;
}
