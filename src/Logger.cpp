#include "Logger.h"

Logger logger;

void Logger::begin(AsyncWebServer &server) {
  events_.onConnect([](AsyncEventSourceClient *client) {
    client->send(logger.history().c_str(), "history", millis());
  });
  server.addHandler(&events_);
}

void Logger::log(const String &message) {
  String line = "[" + String(millis()) + "] " + message;
  Serial.println(line);

  lines_[next_] = line;
  next_ = (next_ + 1) % kCapacity;
  if (count_ < kCapacity) count_++;

  events_.send(line.c_str(), "log", millis());
}

String Logger::history() const {
  String out;
  size_t start = (count_ < kCapacity) ? 0 : next_;
  for (size_t i = 0; i < count_; i++) {
    out += lines_[(start + i) % kCapacity];
    out += "\n";
  }
  return out;
}
