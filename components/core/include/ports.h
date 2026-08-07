#pragma once

#include "domain.h"

class IConfigStore {
public:
  virtual ~IConfigStore() {}
  virtual bool save(const DeviceConfig &cfg) = 0;
  virtual bool load(DeviceConfig &out) = 0;
};

class IFeederDriver {
public:
  virtual ~IFeederDriver() {}
  virtual bool dispense_ms(int ms) = 0;
};

class IFeederRepository {
public:
  virtual ~IFeederRepository() {}
  virtual void addEvent(const FeedEvent &ev) = 0;
  virtual std::vector<FeedEvent> recent(int limit) = 0;
};

class ISensors {
public:
  virtual ~ISensors() {}
  virtual bool readTemperature(float *out) = 0;
  virtual bool readPh(float *out) = 0;
  virtual bool readPpm(float *out) = 0;
};
