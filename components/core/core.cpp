#include "usecases.h"
#include "ports.h"
#include "domain.h"
#include <vector>
#include <string>
#include <mutex>
#include <ctime>
#include <sstream>

// Simple in-memory adapters for scaffold
static DeviceConfig g_config = {"smartFit", 1, 500};
static std::vector<FeedEvent> g_history;
static std::mutex g_mutex;

// Adapters (injected)
static IFeederDriver* g_feeder_driver = nullptr;
static IFeederRepository* g_feeder_repo = nullptr;
static IConfigStore* g_config_store = nullptr;
static ISensors* g_sensors = nullptr;

void Core::setFeederDriver(IFeederDriver* d){ g_feeder_driver = d; }
void Core::setFeederRepository(IFeederRepository* r){ g_feeder_repo = r; }
void Core::setConfigStore(IConfigStore* s){ g_config_store = s; }
void Core::setSensors(ISensors* s){ g_sensors = s; }

static std::string now_iso(){
  std::time_t t = std::time(nullptr);
  char buf[64];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
  return std::string(buf);
}

void Core::init(){
  // load config from adapter if available
  if(g_config_store){
    DeviceConfig tmp;
    if(g_config_store->load(tmp)){
      g_config = tmp;
    }
  }
}

FeedResult Core::feed(int amount){
  std::lock_guard<std::mutex> L(g_mutex);
  // Simplified: compute ms = amount * portion_ms
  int ms = amount * g_config.portion_ms;
  // Call adapter if present
  bool drv_ok = false;
  if(g_feeder_driver){
    drv_ok = g_feeder_driver->dispense_ms(ms);
  } else {
    // fallback: no driver, simulate
    drv_ok = true;
  }
  (void)drv_ok;
  FeedEvent ev; ev.time = now_iso(); ev.action = "feed:" + std::to_string(amount);
  g_history.insert(g_history.begin(), ev);
  if(g_feeder_repo){
    g_feeder_repo->addEvent(ev);
  }
  if(g_history.size()>100) g_history.pop_back();
  return {true, "dispensed"};
}

std::string Core::status_json(){
  std::lock_guard<std::mutex> L(g_mutex);
  std::ostringstream o;
  // add placeholder sensor values (to be replaced by real sensor adapters)
  double temperature = 24.5;
  double ph_val = 7.25;
  const char* next_feed = "16:00 — 500g";
  if(g_sensors){
    float t=0.0f, p=0.0f, ppm=0.0f;
    if(g_sensors->readTemperature(&t)) temperature = t;
    if(g_sensors->readPh(&p)) ph_val = p;
    // ppm available if needed
    (void)ppm;
  }

  o << "{\"state\":\"idle\",\"temperature\":" << temperature << ",\"ph\":" << ph_val << ",\"next_feed\":\"" << next_feed << "\",\"history\":[";
  bool first=true;
  for(auto &ev: g_history){
    if(!first) {
      o<<",";
    }
    first=false;
    o << "{\"time\":\"" << ev.time << "\",\"action\":\"" << ev.action << "\"}";
  }
  o << "]}";
  return o.str();
}
