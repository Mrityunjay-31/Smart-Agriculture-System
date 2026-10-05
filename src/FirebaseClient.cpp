// ═══════════════════════════════════════════════════════════════════════════════
// FirebaseClient.cpp — Firebase Realtime Database REST API client
// Smart Agriculture Monitoring System
//
// Uses libcurl for all HTTP communication with Firebase.
// ═══════════════════════════════════════════════════════════════════════════════

#include "FirebaseClient.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

namespace agri {

// libcurl write callback
static size_t writeCallback(void *contents, size_t size, size_t nmemb,
                            std::string *output) {
  size_t totalSize = size * nmemb;
  output->append(static_cast<char *>(contents), totalSize);
  return totalSize;
}

FirebaseClient::FirebaseClient(const std::string &dbUrl,
                               const std::string &apiKey)
    : dbUrl_(dbUrl), apiKey_(apiKey) {
  // Remove trailing slash from DB URL
  if (!dbUrl_.empty() && dbUrl_.back() == '/') {
    dbUrl_.pop_back();
  }
}

FirebaseClient::~FirebaseClient() = default;

bool FirebaseClient::init() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  connected_ = testConnection();
  return connected_;
}

bool FirebaseClient::testConnection() {
  auto result = get("/.json");
  if (result) {
    connected_ = true;
    std::cout << "[Firebase] Connected to database." << std::endl;
    return true;
  }
  connected_ = false;
  std::cerr << "[Firebase] Connection test failed." << std::endl;
  return false;
}

// ── Sensor Data
// ────────────────────────────────────────────────────────────────

bool FirebaseClient::pushSensorData(const std::string &farmId,
                                    const SensorData &data) {
  std::string path =
      "/sensorData/" + farmId + "/" +
      std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count());
  return put(path, data.toJson());
}

bool FirebaseClient::updateLatest(const std::string &farmId,
                                  const SensorData &data) {
  std::string path = "/latest/" + farmId;
  return put(path, data.toJson());
}

std::optional<SensorData> FirebaseClient::getLatest(const std::string &farmId) {
  auto result = get("/latest/" + farmId);
  if (result && !result->is_null()) {
    return SensorData::fromJson(*result);
  }
  return std::nullopt;
}

std::vector<SensorData> FirebaseClient::getHistory(const std::string &farmId,
                                                   int limitLast) {
  std::string path =
      "/sensorData/" + farmId +
      ".json?orderBy=\"$key\"&limitToLast=" + std::to_string(limitLast);

  // Use raw URL for ordered query
  std::string url = dbUrl_ + path;
  if (!authToken_.empty()) {
    url += "&auth=" + authToken_;
  }

  auto response = httpRequest("GET", url);
  std::vector<SensorData> result;

  if (response) {
    try {
      auto json = nlohmann::json::parse(*response);
      if (!json.is_null() && json.is_object()) {
        for (auto &[key, value] : json.items()) {
          result.push_back(SensorData::fromJson(value));
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "[Firebase] History parse error: " << e.what() << std::endl;
    }
  }

  return result;
}

// ── Alerts
// ─────────────────────────────────────────────────────────────────────

bool FirebaseClient::pushAlert(const std::string &farmId, const Alert &alert) {
  std::string path = "/alerts/" + farmId + "/" + alert.id;
  return put(path, alert.toJson());
}

std::vector<Alert> FirebaseClient::getAlerts(const std::string &farmId) {
  auto result = get("/alerts/" + farmId);
  std::vector<Alert> alerts;

  if (result && !result->is_null() && result->is_object()) {
    for (auto &[key, value] : result->items()) {
      alerts.push_back(Alert::fromJson(value));
    }
  }

  return alerts;
}

bool FirebaseClient::acknowledgeAlert(const std::string &farmId,
                                      const std::string &alertId) {
  std::string path = "/alerts/" + farmId + "/" + alertId;
  return patch(path, {{"acknowledged", true}});
}

// ── System
// ─────────────────────────────────────────────────────────────────────

bool FirebaseClient::updateSystemStatus(const std::string &farmId,
                                        const SystemStatus &status) {
  return put("/system/" + farmId, status.toJson());
}

std::optional<SystemStatus>
FirebaseClient::getSystemStatus(const std::string &farmId) {
  auto result = get("/system/" + farmId);
  if (result && !result->is_null()) {
    SystemStatus s;
    auto &j = *result;
    if (j.contains("arduinoStatus"))
      s.arduinoStatus = j["arduinoStatus"];
    if (j.contains("firebaseStatus"))
      s.firebaseStatus = j["firebaseStatus"];
    if (j.contains("pipelineStatus"))
      s.pipelineStatus = j["pipelineStatus"];
    if (j.contains("lastConnection"))
      s.lastConnection = j["lastConnection"];
    if (j.contains("device"))
      s.device = j["device"];
    if (j.contains("mode"))
      s.mode = j["mode"];
    return s;
  }
  return std::nullopt;
}

// ── Users
// ──────────────────────────────────────────────────────────────────────

bool FirebaseClient::storeUserProfile(const std::string &userId,
                                      const nlohmann::json &profile) {
  return put("/users/" + userId, profile);
}

std::optional<nlohmann::json>
FirebaseClient::getUserProfile(const std::string &userId) {
  return get("/users/" + userId);
}

// ── Farm
// ───────────────────────────────────────────────────────────────────────

bool FirebaseClient::storeFarmInfo(const std::string &farmId,
                                   const nlohmann::json &info) {
  return put("/farms/" + farmId, info);
}

std::optional<nlohmann::json>
FirebaseClient::getFarmInfo(const std::string &farmId) {
  return get("/farms/" + farmId);
}

// ── Generic REST Operations
// ────────────────────────────────────────────────────

std::optional<nlohmann::json> FirebaseClient::get(const std::string &path) {
  std::string url = buildUrl(path);
  auto response = httpRequest("GET", url);

  if (response) {
    try {
      return nlohmann::json::parse(*response);
    } catch (const std::exception &e) {
      std::cerr << "[Firebase] JSON parse error: " << e.what() << std::endl;
    }
  }
  return std::nullopt;
}

bool FirebaseClient::put(const std::string &path, const nlohmann::json &data) {
  std::string url = buildUrl(path);
  auto response = httpRequest("PUT", url, data.dump());
  return response.has_value();
}

bool FirebaseClient::patch(const std::string &path,
                           const nlohmann::json &data) {
  std::string url = buildUrl(path);
  auto response = httpRequest("PATCH", url, data.dump());
  return response.has_value();
}

bool FirebaseClient::remove(const std::string &path) {
  std::string url = buildUrl(path);
  auto response = httpRequest("DELETE", url);
  return response.has_value();
}

std::string FirebaseClient::buildUrl(const std::string &path) const {
  std::string url = dbUrl_ + path;

  // Append .json for Firebase REST API
  if (url.find(".json") == std::string::npos) {
    url += ".json";
  }

  // Append auth token
  if (!authToken_.empty()) {
    url += (url.find('?') != std::string::npos ? "&" : "?");
    url += "auth=" + authToken_;
  }

  return url;
}

std::optional<std::string>
FirebaseClient::httpRequest(const std::string &method, const std::string &url,
                            const std::string &body) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "[Firebase] Failed to initialize curl." << std::endl;
    return std::nullopt;
  }

  std::string responseBody;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  if (method == "PUT") {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  } else if (method == "PATCH") {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  } else if (method == "DELETE") {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  } else if (method == "POST") {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  }
  // GET is the default

  CURLcode res = curl_easy_perform(curl);

  long httpCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "[Firebase] curl error: " << curl_easy_strerror(res)
              << std::endl;
    connected_ = false;
    return std::nullopt;
  }

  if (httpCode >= 400) {
    std::cerr << "[Firebase] HTTP " << httpCode << ": " << responseBody
              << std::endl;
    return std::nullopt;
  }

  connected_ = true;
  return responseBody;
}

} // namespace agri
