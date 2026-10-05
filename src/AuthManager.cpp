// ═══════════════════════════════════════════════════════════════════════════════
// AuthManager.cpp — Firebase Authentication REST API implementation
// Smart Agriculture Monitoring System
// ═══════════════════════════════════════════════════════════════════════════════

#include "AuthManager.hpp"
#include <curl/curl.h>
#include <iostream>
#include <regex>


namespace agri {

static size_t authWriteCallback(void *contents, size_t size, size_t nmemb,
                                std::string *output) {
  size_t totalSize = size * nmemb;
  output->append(static_cast<char *>(contents), totalSize);
  return totalSize;
}

AuthManager::AuthManager(const std::string &apiKey) : apiKey_(apiKey) {}

AuthResult AuthManager::signUp(const std::string &email,
                               const std::string &password,
                               const std::string &displayName) {
  AuthResult result;

  // Validate inputs
  auto emailValidation = validateEmail(email);
  if (!emailValidation.valid) {
    result.errorMessage = emailValidation.error;
    return result;
  }

  auto passValidation = validatePassword(password);
  if (!passValidation.valid) {
    result.errorMessage = passValidation.error;
    return result;
  }

  nlohmann::json body = {
      {"email", email}, {"password", password}, {"returnSecureToken", true}};

  auto response = authRequest(
      "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=" +
          apiKey_,
      body);

  if (!response) {
    result.errorMessage = "Firebase Auth request failed.";
    return result;
  }

  auto &j = *response;

  if (j.contains("error")) {
    result.errorMessage = j["error"]["message"].get<std::string>();
    return result;
  }

  result.success = true;
  result.userId = j.value("localId", "");
  result.idToken = j.value("idToken", "");
  result.refreshToken = j.value("refreshToken", "");
  result.email = j.value("email", "");
  result.expiresIn = std::stoi(j.value("expiresIn", "3600"));

  return result;
}

AuthResult AuthManager::signIn(const std::string &email,
                               const std::string &password) {
  AuthResult result;

  nlohmann::json body = {
      {"email", email}, {"password", password}, {"returnSecureToken", true}};

  auto response = authRequest("https://identitytoolkit.googleapis.com/v1/"
                              "accounts:signInWithPassword?key=" +
                                  apiKey_,
                              body);

  if (!response) {
    result.errorMessage = "Firebase Auth request failed.";
    return result;
  }

  auto &j = *response;

  if (j.contains("error")) {
    result.errorMessage = j["error"]["message"].get<std::string>();
    return result;
  }

  result.success = true;
  result.userId = j.value("localId", "");
  result.idToken = j.value("idToken", "");
  result.refreshToken = j.value("refreshToken", "");
  result.email = j.value("email", "");
  result.expiresIn = std::stoi(j.value("expiresIn", "3600"));

  return result;
}

bool AuthManager::forgotPassword(const std::string &email) {
  nlohmann::json body = {{"requestType", "PASSWORD_RESET"}, {"email", email}};

  auto response = authRequest(
      "https://identitytoolkit.googleapis.com/v1/accounts:sendOobCode?key=" +
          apiKey_,
      body);

  return response.has_value() && !response->contains("error");
}

AuthResult AuthManager::refreshIdToken(const std::string &refreshToken) {
  AuthResult result;

  nlohmann::json body = {{"grant_type", "refresh_token"},
                         {"refresh_token", refreshToken}};

  auto response = authRequest(
      "https://securetoken.googleapis.com/v1/token?key=" + apiKey_, body);

  if (!response) {
    result.errorMessage = "Token refresh request failed.";
    return result;
  }

  auto &j = *response;

  if (j.contains("error")) {
    result.errorMessage = j["error"]["message"].get<std::string>();
    return result;
  }

  result.success = true;
  result.userId = j.value("user_id", "");
  result.idToken = j.value("id_token", "");
  result.refreshToken = j.value("refresh_token", "");
  result.expiresIn = std::stoi(j.value("expires_in", "3600"));

  return result;
}

// ── Validation
// ─────────────────────────────────────────────────────────────────

AuthManager::ValidationResult
AuthManager::validateEmail(const std::string &email) {
  ValidationResult r;

  if (email.empty()) {
    r.valid = false;
    r.error = "Email is required.";
    return r;
  }

  static const std::regex emailRegex(
      R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
  if (!std::regex_match(email, emailRegex)) {
    r.valid = false;
    r.error = "Invalid email format.";
    return r;
  }

  return r;
}

AuthManager::ValidationResult
AuthManager::validatePassword(const std::string &password) {
  ValidationResult r;

  if (password.empty()) {
    r.valid = false;
    r.error = "Password is required.";
    return r;
  }

  if (password.size() < 6) {
    r.valid = false;
    r.error = "Password must be at least 6 characters.";
    return r;
  }

  return r;
}

AuthManager::ValidationResult AuthManager::validateSignUpFields(
    const std::string &name, const std::string &email,
    const std::string &password, const std::string &confirmPassword) {

  ValidationResult r;

  if (name.empty()) {
    r.valid = false;
    r.error = "Full name is required.";
    return r;
  }

  auto emailR = validateEmail(email);
  if (!emailR.valid)
    return emailR;

  auto passR = validatePassword(password);
  if (!passR.valid)
    return passR;

  if (password != confirmPassword) {
    r.valid = false;
    r.error = "Passwords do not match.";
    return r;
  }

  return r;
}

// ── Private
// ────────────────────────────────────────────────────────────────────

std::optional<nlohmann::json>
AuthManager::authRequest(const std::string &endpoint,
                         const nlohmann::json &body) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return std::nullopt;

  std::string responseBody;
  std::string requestBody = body.dump();

  curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, authWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode res = curl_easy_perform(curl);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "[Auth] curl error: " << curl_easy_strerror(res) << std::endl;
    return std::nullopt;
  }

  try {
    return nlohmann::json::parse(responseBody);
  } catch (...) {
    return std::nullopt;
  }
}

} // namespace agri
