// ═══════════════════════════════════════════════════════════════════════════════
// SerialPort.cpp — Linux POSIX Serial Communication Implementation
// Smart Agriculture Monitoring System
//
// This file demonstrates Linux system programming concepts:
//   - Character device I/O via /dev/ttyACM0 or /dev/ttyUSB0
//   - File descriptor management with RAII
//   - termios configuration for serial parameters
//   - select() system call for non-blocking reads with timeout
//   - Graceful device disconnection and reconnection
// ═══════════════════════════════════════════════════════════════════════════════

#include "SerialPort.hpp"

// Linux-specific headers for serial communication
#ifdef __linux__
#include <cstring>      // strerror()
#include <errno.h>      // errno, EAGAIN, EINTR
#include <fcntl.h>      // open(), O_RDWR, O_NOCTTY, O_NONBLOCK
#include <sys/ioctl.h>  // ioctl() — optional modem line control
#include <sys/select.h> // select(), fd_set, FD_ZERO, FD_SET
#include <termios.h> // struct termios, tcgetattr(), tcsetattr(), cfsetispeed()
#include <unistd.h>  // read(), write(), close()

#endif

#include <chrono>
#include <iostream>
#include <thread>


namespace agri {

SerialPort::SerialPort(const std::string &device, int baud)
    : device_(device), baud_(baud), fd_(-1) {}

SerialPort::~SerialPort() { close(); }

SerialPort::SerialPort(SerialPort &&other) noexcept
    : device_(std::move(other.device_)), baud_(other.baud_), fd_(other.fd_),
      lineBuffer_(std::move(other.lineBuffer_)) {
  other.fd_ = -1;
}

SerialPort &SerialPort::operator=(SerialPort &&other) noexcept {
  if (this != &other) {
    close();
    device_ = std::move(other.device_);
    baud_ = other.baud_;
    fd_ = other.fd_;
    lineBuffer_ = std::move(other.lineBuffer_);
    other.fd_ = -1;
  }
  return *this;
}

bool SerialPort::open() {
#ifdef __linux__
  // Open the character device with:
  //   O_RDWR    — Read and write access
  //   O_NOCTTY  — Don't make this the controlling terminal
  //   O_NONBLOCK — Non-blocking open (to avoid hanging if device is busy)
  fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (fd_ < 0) {
    std::cerr << "[SerialPort] ERROR: Cannot open " << device_ << ": "
              << strerror(errno) << std::endl;
    return false;
  }

  // Clear the non-blocking flag after opening (we'll use select() for timeout)
  int flags = fcntl(fd_, F_GETFL, 0);
  fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);

  // Configure termios for serial communication
  if (!configureTermios()) {
    ::close(fd_);
    fd_ = -1;
    return false;
  }

  // Flush any stale data in the serial buffers
  flush();

  std::cout << "[SerialPort] Opened " << device_ << " at " << baud_ << " baud"
            << std::endl;
  return true;
#else
  std::cerr << "[SerialPort] ERROR: Serial port requires Linux." << std::endl;
  return false;
#endif
}

void SerialPort::close() {
#ifdef __linux__
  if (fd_ >= 0) {
    ::close(fd_);
    std::cout << "[SerialPort] Closed " << device_ << std::endl;
    fd_ = -1;
  }
#endif
}

bool SerialPort::isOpen() const { return fd_ >= 0; }

std::string SerialPort::readLine(int timeoutMs) {
#ifdef __linux__
  if (fd_ < 0)
    return "";

  // Use select() for non-blocking read with timeout
  // This demonstrates the Linux select() system call for I/O multiplexing
  fd_set readfds;
  struct timeval tv;

  auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

  while (std::chrono::steady_clock::now() < deadline) {
    // Check if we already have a complete line in the buffer
    auto newlinePos = lineBuffer_.find('\n');
    if (newlinePos != std::string::npos) {
      std::string line = lineBuffer_.substr(0, newlinePos);
      lineBuffer_ = lineBuffer_.substr(newlinePos + 1);
      // Remove trailing \r if present
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      return line;
    }

    // Set up select() to wait for data on the serial file descriptor
    FD_ZERO(&readfds);
    FD_SET(fd_, &readfds);

    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now());

    if (remaining.count() <= 0)
      break;

    tv.tv_sec = remaining.count() / 1000;
    tv.tv_usec = (remaining.count() % 1000) * 1000;

    int ret = select(fd_ + 1, &readfds, nullptr, nullptr, &tv);

    if (ret < 0) {
      if (errno == EINTR)
        continue; // Interrupted by signal, retry
      std::cerr << "[SerialPort] select() error: " << strerror(errno)
                << std::endl;
      return "";
    }

    if (ret == 0) {
      // Timeout — no data available
      continue;
    }

    // Data available — read from the file descriptor
    char buf[256];
    ssize_t n = ::read(fd_, buf, sizeof(buf) - 1);

    if (n < 0) {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      std::cerr << "[SerialPort] read() error: " << strerror(errno)
                << std::endl;
      return "";
    }

    if (n == 0) {
      // Device disconnected (EOF)
      std::cerr << "[SerialPort] Device disconnected." << std::endl;
      return "";
    }

    // Append read bytes to the line buffer
    buf[n] = '\0';
    lineBuffer_.append(buf, n);
  }

  return ""; // Timeout without complete line
#else
  return "";
#endif
}

int SerialPort::writeLine(const std::string &data) {
#ifdef __linux__
  if (fd_ < 0)
    return -1;
  std::string toWrite = data + "\n";
  return static_cast<int>(::write(fd_, toWrite.c_str(), toWrite.size()));
#else
  return -1;
#endif
}

bool SerialPort::reconnect() {
  std::cout << "[SerialPort] Attempting reconnection to " << device_ << "..."
            << std::endl;
  close();

  // Wait briefly before reconnecting
  std::this_thread::sleep_for(std::chrono::seconds(2));

  return open();
}

void SerialPort::flush() {
#ifdef __linux__
  if (fd_ >= 0) {
    // tcflush() discards data in the serial input/output buffers
    tcflush(fd_, TCIOFLUSH);
  }
#endif
  lineBuffer_.clear();
}

bool SerialPort::configureTermios() {
#ifdef __linux__
  struct termios tty;

  // Get current terminal attributes
  if (tcgetattr(fd_, &tty) != 0) {
    std::cerr << "[SerialPort] tcgetattr() failed: " << strerror(errno)
              << std::endl;
    return false;
  }

  // ── Input speed and output speed ──────────────────────────────────────
  speed_t speed = baudToConstant(baud_);
  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  // ── Control flags (c_cflag) ───────────────────────────────────────────
  tty.c_cflag &= ~PARENB; // No parity
  tty.c_cflag &= ~CSTOPB; // 1 stop bit
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;      // 8 data bits
  tty.c_cflag &= ~CRTSCTS; // No hardware flow control
  tty.c_cflag |= CREAD;    // Enable receiver
  tty.c_cflag |= CLOCAL;   // Ignore modem control lines

  // ── Input flags (c_iflag) ─────────────────────────────────────────────
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
  tty.c_iflag |= IGNPAR; // Ignore parity errors

  // ── Output flags (c_oflag) ────────────────────────────────────────────
  tty.c_oflag &= ~OPOST; // Raw output
  tty.c_oflag &= ~ONLCR; // Don't convert \n to \r\n

  // ── Local flags (c_lflag) ─────────────────────────────────────────────
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN); // Raw mode

  // ── Special characters ────────────────────────────────────────────────
  tty.c_cc[VMIN] = 0;   // Non-blocking read
  tty.c_cc[VTIME] = 10; // 1 second timeout (in tenths of a second)

  // Apply the configuration immediately (TCSANOW)
  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    std::cerr << "[SerialPort] tcsetattr() failed: " << strerror(errno)
              << std::endl;
    return false;
  }

  return true;
#else
  return false;
#endif
}

int SerialPort::baudToConstant(int baud) {
#ifdef __linux__
  switch (baud) {
  case 1200:
    return B1200;
  case 2400:
    return B2400;
  case 4800:
    return B4800;
  case 9600:
    return B9600;
  case 19200:
    return B19200;
  case 38400:
    return B38400;
  case 57600:
    return B57600;
  case 115200:
    return B115200;
  case 230400:
    return B230400;
  case 460800:
    return B460800;
  default:
    return B9600; // Default to 9600
  }
#else
  return 0;
#endif
}

} // namespace agri
