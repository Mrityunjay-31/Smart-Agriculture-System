// ═══════════════════════════════════════════════════════════════════════════════
// SessionManager.cpp — In-memory session management
// ═══════════════════════════════════════════════════════════════════════════════

#include "SessionManager.hpp"
#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>


namespace agri {

std::string SessionManager::createSession(const std::string &userId,
                                          const std::string &idToken,
                                          const std::string &refreshToken,
                                          const std::string &email,
                                          const std::string &displayName,
                                          int expiresInSeconds) {
  std::lock_guard<std::mutex> lock(mutex_);

  Session session;
  session.sessionToken = generateToken();
  session.userId = userId;
  session.idToken = idToken;
  session.refreshToken = refreshToken;
  session.email = email;
  session.displayName = displayName;
  session.createdAt = std::chrono::steady_clock::now();
  session.expiresAt =
      session.createdAt + std::chrono::seconds(expiresInSeconds);

  sessions_[session.sessionToken] = session;
  return session.sessionToken;
}

std::optional<Session>
SessionManager::getSession(const std::string &sessionToken) const {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = sessions_.find(sessionToken);
  if (it == sessions_.end())
    return std::nullopt;

  // Check expiry
  if (std::chrono::steady_clock::now() > it->second.expiresAt) {
    return std::nullopt;
  }

  return it->second;
}

void SessionManager::removeSession(const std::string &sessionToken) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_.erase(sessionToken);
}

void SessionManager::cleanup() {
  std::lock_guard<std::mutex> lock(mutex_);
  auto now = std::chrono::steady_clock::now();

  for (auto it = sessions_.begin(); it != sessions_.end();) {
    if (now > it->second.expiresAt) {
      it = sessions_.erase(it);
    } else {
      ++it;
    }
  }
}

bool SessionManager::isValid(const std::string &sessionToken) const {
  return getSession(sessionToken).has_value();
}

std::optional<std::string>
SessionManager::getUserId(const std::string &sessionToken) const {
  auto session = getSession(sessionToken);
  if (session)
    return session->userId;
  return std::nullopt;
}

std::string SessionManager::generateToken() {
  static std::mt19937 rng(std::random_device{}());
  static std::uniform_int_distribution<uint64_t> dist;

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  oss << std::setw(16) << dist(rng);
  oss << std::setw(16) << dist(rng);
  return oss.str();
}

} // namespace agri
