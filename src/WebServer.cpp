// ═══════════════════════════════════════════════════════════════════════════════
// WebServer.cpp — Crow HTTP server implementation
// Smart Agriculture Monitoring System
//
// Provides:
//   - Static HTML page serving (dashboard, login, etc.)
//   - REST API endpoints for sensors, alerts, auth
//   - Session-based authentication middleware
// ═══════════════════════════════════════════════════════════════════════════════

#include "WebServer.hpp"
#include "SensorParser.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace agri {

WebServer::WebServer(DataPipeline &pipeline, SensorProcessor &processor,
                     FirebaseClient &firebase, AuthManager &auth,
                     AlertManager &alerts, SessionManager &sessions,
                     ConfigManager &config)
    : pipeline_(pipeline), processor_(processor), firebase_(firebase),
      auth_(auth), alerts_(alerts), sessions_(sessions), config_(config) {}

void WebServer::start(int port, const std::string &host) {
  setupRoutes();

  std::cout << "═══════════════════════════════════════════════════"
            << std::endl;
  std::cout << "  Smart Agriculture Web Server" << std::endl;
  std::cout << "  http://" << host << ":" << port << std::endl;
  std::cout << "═══════════════════════════════════════════════════"
            << std::endl;

  app_.port(port).bindaddr(host).multithreaded().run();
}

void WebServer::stop() { app_.stop(); }

void WebServer::setupRoutes() {
  std::cout << "\n\n====> [DEBUG] WebServer::setupRoutes() IS BEING EXECUTED! "
               "<====\n\n";
  setupPageRoutes();
  setupAuthRoutes();
  setupSensorRoutes();
  setupTinkercadIngestRoutes();
  setupAlertRoutes();
  setupSystemRoutes();
  setupFarmRoutes();
  setupSettingsRoutes();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Page Routes
// ═══════════════════════════════════════════════════════════════════════════════

void WebServer::setupPageRoutes() {
  // Root → redirect to dashboard or login
  CROW_ROUTE(app_, "/").methods(crow::HTTPMethod::GET)(
      [this](const crow::request &req) {
        auto session = authenticateRequest(req);
        crow::response res;
        if (session) {
          res.code = 302;
          res.set_header("Location", "/dashboard");
        } else {
          res.code = 302;
          res.set_header("Location", "/login");
        }
        return res;
      });

  CROW_ROUTE(app_, "/login")
  ([this](const crow::request &) {
    auto html = loadTemplate("login.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/signup")
  ([this](const crow::request &) {
    auto html = loadTemplate("signup.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/forgot-password")
  ([this](const crow::request &) {
    auto html = loadTemplate("forgot-password.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/dashboard")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return redirectToLogin();
    auto html = loadTemplate("dashboard.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/sensors")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return redirectToLogin();
    auto html = loadTemplate("sensors.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/alerts")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return redirectToLogin();
    auto html = loadTemplate("alerts.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/reports")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return redirectToLogin();
    auto html = loadTemplate("reports.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/farm")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return redirectToLogin();
    auto html = loadTemplate("farm.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/profile")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return redirectToLogin();
    auto html = loadTemplate("profile.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/settings")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return redirectToLogin();
    auto html = loadTemplate("settings.html");
    return crow::response(200, "text/html", html);
  });

  CROW_ROUTE(app_, "/logout")
  ([this](const crow::request &req) {
    // Extract session token from cookie
    std::string cookie = req.get_header_value("Cookie");
    std::string token;
    auto pos = cookie.find("session_token=");
    if (pos != std::string::npos) {
      auto start = pos + 14;
      auto end = cookie.find(';', start);
      token = cookie.substr(start, end - start);
    }

    if (!token.empty()) {
      sessions_.removeSession(token);
    }

    crow::response res(302);
    res.set_header("Location", "/login");
    res.set_header("Set-Cookie", "session_token=; Path=/; Max-Age=0");
    return res;
  });
}

// ═══════════════════════════════════════════════════════════════════════════════
// Auth API Routes
// ═══════════════════════════════════════════════════════════════════════════════

void WebServer::setupAuthRoutes() {
  CROW_ROUTE(app_, "/api/auth/signup")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        try {
          auto body = nlohmann::json::parse(req.body);

          std::string name = body.value("name", "");
          std::string email = body.value("email", "");
          std::string password = body.value("password", "");
          std::string confirm = body.value("confirmPassword", "");

          // Validate
          auto validation =
              AuthManager::validateSignUpFields(name, email, password, confirm);
          if (!validation.valid) {
            return jsonError(400, validation.error);
          }

          // Create user with Firebase Auth
          auto result = auth_.signUp(email, password, name);
          if (!result.success) {
            return jsonError(400, result.errorMessage);
          }

          // Store user profile in Firebase RTDB
          firebase_.setAuthToken(result.idToken);
          firebase_.storeUserProfile(
              result.userId, {{"name", name},
                              {"email", email},
                              {"role", "user"},
                              {"createdAt", SensorData::currentTimestamp()}});

          // Create session
          std::string sessionToken = sessions_.createSession(
              result.userId, result.idToken, result.refreshToken, email, name,
              result.expiresIn);

          crow::response res(200);
          res.set_header("Content-Type", "application/json");
          res.set_header("Set-Cookie", "session_token=" + sessionToken +
                                           "; Path=/; HttpOnly; Max-Age=" +
                                           std::to_string(result.expiresIn));
          res.write(
              nlohmann::json({{"success", true}, {"redirect", "/dashboard"}})
                  .dump());
          return res;

        } catch (const std::exception &e) {
          return jsonError(400, std::string("Invalid request: ") + e.what());
        }
      });

  CROW_ROUTE(app_, "/api/auth/login")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        try {
          auto body = nlohmann::json::parse(req.body);

          std::string email = body.value("email", "");
          std::string password = body.value("password", "");

          if (email.empty() || password.empty()) {
            return jsonError(400, "Email and password are required.");
          }

          auto result = auth_.signIn(email, password);
          if (!result.success) {
            return jsonError(401, result.errorMessage);
          }

          // Set auth token for Firebase RTDB access
          firebase_.setAuthToken(result.idToken);

          // Get display name from profile
          std::string displayName = email;
          auto profile = firebase_.getUserProfile(result.userId);
          if (profile && profile->contains("name")) {
            displayName = (*profile)["name"].get<std::string>();
          }

          // Create session
          std::string sessionToken = sessions_.createSession(
              result.userId, result.idToken, result.refreshToken, email,
              displayName, result.expiresIn);

          crow::response res(200);
          res.set_header("Content-Type", "application/json");
          res.set_header("Set-Cookie", "session_token=" + sessionToken +
                                           "; Path=/; HttpOnly; Max-Age=" +
                                           std::to_string(result.expiresIn));
          res.write(
              nlohmann::json({{"success", true}, {"redirect", "/dashboard"}})
                  .dump());
          return res;

        } catch (const std::exception &e) {
          return jsonError(400, std::string("Invalid request: ") + e.what());
        }
      });

  CROW_ROUTE(app_, "/api/auth/forgot-password")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        try {
          auto body = nlohmann::json::parse(req.body);
          std::string email = body.value("email", "");

          if (email.empty()) {
            return jsonError(400, "Email is required.");
          }

          bool sent = auth_.forgotPassword(email);
          if (sent) {
            return crow::response(
                200, "application/json",
                nlohmann::json({{"success", true},
                                {"message", "Password reset email sent."}})
                    .dump());
          } else {
            return jsonError(400, "Failed to send password reset email.");
          }
        } catch (const std::exception &e) {
          return jsonError(400, std::string("Invalid request: ") + e.what());
        }
      });

  CROW_ROUTE(app_, "/api/auth/session")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session) {
      return jsonError(401, "Not authenticated.");
    }

    return crow::response(
        200, "application/json",
        nlohmann::json({{"authenticated", true},
                        {"userId", session->userId},
                        {"email", session->email},
                        {"displayName", session->displayName}})
            .dump());
  });
}

// ═══════════════════════════════════════════════════════════════════════════════
// Sensor API Routes
// ═══════════════════════════════════════════════════════════════════════════════

void WebServer::setupSensorRoutes() {
  CROW_ROUTE(app_, "/api/sensors/manual")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        auto session = authenticateRequest(req);
        if (!session)
          return jsonError(401, "Not authenticated.");

        try {
          auto body = nlohmann::json::parse(req.body);
          if (body.contains("raw_string")) {
            std::string raw = body["raw_string"];
            SensorParser parser;
            auto parsedOpt = parser.parseDataLine(raw);
            if (parsedOpt) {
              processor_.process(*parsedOpt);
              return crow::response(200, "application/json",
                                    nlohmann::json({{"success", true}}).dump());
            } else {
              return jsonError(
                  400, "Invalid Arduino data string format. Expected: 'Temp: "
                       "24.7C | Hum: 42% | Soil: 47% | pH: 7.3 | Light: 100%'");
            }
          }
          return jsonError(400, "Missing raw_string");
        } catch (const std::exception &e) {
          return jsonError(400, std::string("Parse error: ") + e.what());
        }
      });

  CROW_ROUTE(app_, "/api/sensors/latest")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return jsonError(401, "Not authenticated.");

    std::string farmId = config_.getFarmId();

    // First try in-memory latest (fastest)
    if (processor_.hasData()) {
      auto data = processor_.getLatest();
      return crow::response(200, "application/json", data.toJson().dump());
    }

    // Fallback to Firebase
    auto data = firebase_.getLatest(farmId);
    if (data) {
      return crow::response(200, "application/json", data->toJson().dump());
    }

    return jsonError(404, "No sensor data available.");
  });

  CROW_ROUTE(app_, "/api/sensors/history")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return jsonError(401, "Not authenticated.");

    std::string farmId = config_.getFarmId();

    // Get limit from query string
    int limit = 100;
    auto limitParam = req.url_params.get("limit");
    if (limitParam) {
      try {
        limit = std::stoi(limitParam);
      } catch (...) {
      }
    }

    auto history = firebase_.getHistory(farmId, limit);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto &d : history) {
      arr.push_back(d.toJson());
    }

    return crow::response(200, "application/json", arr.dump());
  });
}

// ═══════════════════════════════════════════════════════════════════════════════
// Tinkercad Ingestion API Routes
// ═══════════════════════════════════════════════════════════════════════════════

void WebServer::setupTinkercadIngestRoutes() {
  CROW_ROUTE(app_, "/api/ingest/tinkercad")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        // 1. Verify token
        std::string expectedToken = config_.getTinkercadBridgeToken();
        if (expectedToken.empty()) {
          return jsonError(500,
                           "Tinkercad bridge token not configured on server.");
        }

        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.find("Bearer ") == 0) {
          std::string token = authHeader.substr(7);
          if (token != expectedToken) {
            return jsonError(401, "Invalid authorization token.");
          }
        } else {
          return jsonError(401, "Missing or malformed Authorization header.");
        }

        // 2. Parse and inject
        try {
          auto body = nlohmann::json::parse(req.body);
          if (body.contains("raw_line")) {
            std::string rawLine = body["raw_line"];
            std::string farmId = config_.getFarmId();

            // This relies on the FULL DataPipeline:
            // parsing -> validation -> process (threshold check) -> alerts ->
            // firebase
            pipeline_.processReading(rawLine, "", farmId);

            return crow::response(
                200, "application/json",
                nlohmann::json(
                    {{"success", true}, {"message", "Data pushed to pipeline"}})
                    .dump());
          }
          return jsonError(400, "Missing 'raw_line' field in JSON body.");
        } catch (const std::exception &e) {
          return jsonError(400, std::string("Malformed JSON: ") + e.what());
        }
      });
}

// ═══════════════════════════════════════════════════════════════════════════════
// Alert API Routes
// ═══════════════════════════════════════════════════════════════════════════════

void WebServer::setupAlertRoutes() {
  CROW_ROUTE(app_, "/api/alerts")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return jsonError(401, "Not authenticated.");

    auto alerts = alerts_.getRecent(50);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto &a : alerts) {
      arr.push_back(a.toJson());
    }

    return crow::response(200, "application/json", arr.dump());
  });

  CROW_ROUTE(app_, "/api/alerts/acknowledge")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        auto session = authenticateRequest(req);
        if (!session)
          return jsonError(401, "Not authenticated.");

        try {
          auto body = nlohmann::json::parse(req.body);
          std::string alertId = body.value("alertId", "");

          if (alertId.empty()) {
            return jsonError(400, "Alert ID is required.");
          }

          bool ok = alerts_.acknowledge(alertId);

          // Also update in Firebase
          std::string farmId = config_.getFarmId();
          firebase_.acknowledgeAlert(farmId, alertId);

          return crow::response(200, "application/json",
                                nlohmann::json({{"success", ok}}).dump());
        } catch (const std::exception &e) {
          return jsonError(400, std::string("Invalid request: ") + e.what());
        }
      });
}

// ═══════════════════════════════════════════════════════════════════════════════
// System API Routes
// ═══════════════════════════════════════════════════════════════════════════════

void WebServer::setupSystemRoutes() {
  CROW_ROUTE(app_, "/api/system/status")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return jsonError(401, "Not authenticated.");

    // Build system status from config and live state
    auto pipeStatus = pipeline_.getSystemStatus();
    nlohmann::json status = {{"firebaseConnected", firebase_.isConnected()},
                             {"mode", config_.getAppMode()},
                             {"arduinoStatus", pipeStatus.arduinoStatus},
                             {"device", config_.getSerialDevice()},
                             {"farmId", config_.getFarmId()},
                             {"farmName", config_.getFarmName()},
                             {"refreshMs", config_.getDashboardRefreshMs()},
                             {"timestamp", SensorData::currentTimestamp()}};

    return crow::response(200, "application/json", status.dump());
  });
}

// ═══════════════════════════════════════════════════════════════════════════════
// Farm API Routes
// ═══════════════════════════════════════════════════════════════════════════════

void WebServer::setupFarmRoutes() {
  CROW_ROUTE(app_, "/api/farm")
      .methods(crow::HTTPMethod::GET,
               crow::HTTPMethod::POST)([this](const crow::request &req) {
        auto session = authenticateRequest(req);
        if (!session)
          return jsonError(401, "Not authenticated.");

        if (req.method == crow::HTTPMethod::GET) {
          std::string farmId = config_.getFarmId();
          auto info = firebase_.getFarmInfo(farmId);

          if (info) {
            return crow::response(200, "application/json", info->dump());
          }

          // Return defaults
          nlohmann::json defaults = {{"id", farmId},
                                     {"name", config_.getFarmName()},
                                     {"location", config_.getFarmLocation()},
                                     {"cropType", config_.getFarmCrop()}};
          return crow::response(200, "application/json", defaults.dump());
        } else {
          try {
            auto body = nlohmann::json::parse(req.body);
            std::string farmId = config_.getFarmId();

            firebase_.storeFarmInfo(farmId, body);

            // Update local config
            if (body.contains("name"))
              config_.set("farm.name", body["name"]);
            if (body.contains("location"))
              config_.set("farm.location", body["location"]);
            if (body.contains("cropType"))
              config_.set("farm.crop", body["cropType"]);

            return crow::response(200, "application/json",
                                  nlohmann::json({{"success", true}}).dump());
          } catch (const std::exception &e) {
            return jsonError(400, std::string("Invalid request: ") + e.what());
          }
        }
      });
}

// ═══════════════════════════════════════════════════════════════════════════════
// Settings API Routes
// ═══════════════════════════════════════════════════════════════════════════════

void WebServer::setupSettingsRoutes() {
  CROW_ROUTE(app_, "/api/settings")
  ([this](const crow::request &req) {
    auto session = authenticateRequest(req);
    if (!session)
      return jsonError(401, "Not authenticated.");

    nlohmann::json settings = {
        {"serialDevice", config_.getSerialDevice()},
        {"baudRate", config_.getSerialBaud()},
        {"refreshInterval", config_.getDashboardRefreshMs()},
        {"appMode", config_.getAppMode()},
        {"serverPort", config_.getServerPort()},
        {"farmId", config_.getFarmId()},
        {"farmName", config_.getFarmName()},
        {"farmLocation", config_.getFarmLocation()},
        {"farmCrop", config_.getFarmCrop()}};

    return crow::response(200, "application/json", settings.dump());
  });
}

// ═══════════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════════

std::string WebServer::loadTemplate(const std::string &filename) {
  std::string path = "web/templates/" + filename;
  std::ifstream file(path);

  if (!file.is_open()) {
    return "<html><body><h1>Template not found: " + filename +
           "</h1></body></html>";
  }

  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

std::optional<Session>
WebServer::authenticateRequest(const crow::request &req) {
  std::string cookie = req.get_header_value("Cookie");

  // Parse session_token from cookie header
  std::string token;
  auto pos = cookie.find("session_token=");
  if (pos != std::string::npos) {
    auto start = pos + 14; // length of "session_token="
    auto end = cookie.find(';', start);
    token = cookie.substr(
        start, (end != std::string::npos) ? end - start : std::string::npos);
  }

  if (token.empty())
    return std::nullopt;

  return sessions_.getSession(token);
}

crow::response WebServer::redirectToLogin() {
  crow::response res(302);
  res.set_header("Location", "/login");
  return res;
}

crow::response WebServer::jsonError(int code, const std::string &message) {
  return crow::response(
      code, "application/json",
      nlohmann::json({{"error", true}, {"message", message}}).dump());
}

} // namespace agri
