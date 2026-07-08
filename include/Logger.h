#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Ring buffer of recent log lines, mirrored to Serial and streamed live to
// any connected browser via Server-Sent Events at /events.
class Logger {
 public:
  void begin(AsyncWebServer &server);
  void log(const String &message);

  // Newline-joined snapshot of the buffered history, oldest first.
  String history() const;

 private:
  static constexpr size_t kCapacity = 200;

  String lines_[kCapacity];
  size_t next_ = 0;
  size_t count_ = 0;
  AsyncEventSource events_{"/events"};
};

extern Logger logger;
