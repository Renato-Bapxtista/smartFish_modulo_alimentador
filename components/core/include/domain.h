#pragma once

#include <string>
#include <vector>

struct FeedEvent {
  std::string time; // ISO string
  std::string action;
};

struct DeviceConfig {
  std::string name;
  int portions_per_cycle;
  int portion_ms;
};
