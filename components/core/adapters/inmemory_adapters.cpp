#include "ports.h"
#include <vector>
#include <string>

class InMemoryConfigStore : public IConfigStore {
public:
  virtual bool save(const DeviceConfig &cfg) override{
    stored = cfg; return true;
  }
  virtual bool load(DeviceConfig &out) override{
    out = stored; return true;
  }
private:
  DeviceConfig stored {"smartFit",1,500};
};

class InMemoryFeederRepo : public IFeederRepository {
public:
  virtual void addEvent(const FeedEvent &ev) override{ data.insert(data.begin(), ev); if(data.size()>200) data.pop_back(); }
  virtual std::vector<FeedEvent> recent(int limit) override{ if(limit<0) limit = data.size(); std::vector<FeedEvent> out; for(size_t i=0;i<data.size() && (int)out.size()<limit;++i) out.push_back(data[i]); return out; }
private:
  std::vector<FeedEvent> data;
};

IConfigStore* create_inmemory_config_store(){ static InMemoryConfigStore s; return &s; }
IFeederRepository* create_inmemory_repo(){ static InMemoryFeederRepo r; return &r; }
