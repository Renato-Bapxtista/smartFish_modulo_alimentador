#pragma once

#include "domain.h"

struct FeedResult {
  bool ok;
  std::string message;
};

class Core {
public:
  static void init();
  static FeedResult feed(int amount);
  static std::string status_json();
  // Adapter injection
  static void setFeederDriver(class IFeederDriver* d);
  static void setFeederRepository(class IFeederRepository* r);
  static void setConfigStore(class IConfigStore* s);
  static void setSensors(class ISensors* s);
};
