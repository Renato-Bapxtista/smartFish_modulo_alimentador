#include "ports.h"
#include <string>
#include <vector>
#include <cstdio>

#if __has_include(<sqlite3.h>)
#include <sqlite3.h>
#endif

#if __has_include(<sqlite3.h>)
class SQLiteConfigStore : public IConfigStore {
public:
  SQLiteConfigStore(const char* path){
    if(sqlite3_open(path, &db) != SQLITE_OK){ db = nullptr; }
    if(db){
      const char *sql = "CREATE TABLE IF NOT EXISTS config (id INTEGER PRIMARY KEY, name TEXT, portions_per_cycle INTEGER, portion_ms INTEGER);";
      sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
  }
  virtual ~SQLiteConfigStore(){ if(db) sqlite3_close(db); }
  virtual bool save(const DeviceConfig &cfg) override{
    if(!db) return false;
    const char *sql = "INSERT OR REPLACE INTO config (id,name,portions_per_cycle,portion_ms) VALUES (1,?, ?, ?);";
    sqlite3_stmt *stmt=nullptr;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, cfg.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, cfg.portions_per_cycle);
    sqlite3_bind_int(stmt, 3, cfg.portion_ms);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
  }
  virtual bool load(DeviceConfig &out) override{
    if(!db) return false;
    const char *sql = "SELECT name,portions_per_cycle,portion_ms FROM config WHERE id=1 LIMIT 1;";
    sqlite3_stmt *stmt=nullptr;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    int rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW){
      const unsigned char *name = sqlite3_column_text(stmt,0);
      out.name = name ? reinterpret_cast<const char*>(name) : std::string();
      out.portions_per_cycle = sqlite3_column_int(stmt,1);
      out.portion_ms = sqlite3_column_int(stmt,2);
      sqlite3_finalize(stmt);
      return true;
    }
    sqlite3_finalize(stmt);
    return false;
  }
private:
  sqlite3 *db;
};

class SQLiteFeederRepo : public IFeederRepository {
public:
  SQLiteFeederRepo(const char* path){
    if(sqlite3_open(path, &db) != SQLITE_OK){ db=nullptr; }
    if(db){
      const char *sql = "CREATE TABLE IF NOT EXISTS feed_events (id INTEGER PRIMARY KEY AUTOINCREMENT, time TEXT, action TEXT);";
      sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
  }
  virtual ~SQLiteFeederRepo(){ if(db) sqlite3_close(db); }
  virtual void addEvent(const FeedEvent &ev) override{
    if(!db) return;
    const char *sql = "INSERT INTO feed_events (time,action) VALUES (?,?);";
    sqlite3_stmt *stmt=nullptr;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt,1, ev.time.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,2, ev.action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }
  virtual std::vector<FeedEvent> recent(int limit) override{
    std::vector<FeedEvent> out;
    if(!db) return out;
    const char *sql = "SELECT time,action FROM feed_events ORDER BY id DESC LIMIT ?;";
    sqlite3_stmt *stmt=nullptr;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int(stmt, 1, limit);
    while(sqlite3_step(stmt) == SQLITE_ROW){
      const unsigned char* t = sqlite3_column_text(stmt,0);
      const unsigned char* a = sqlite3_column_text(stmt,1);
      FeedEvent ev; ev.time = t ? reinterpret_cast<const char*>(t) : std::string(); ev.action = a ? reinterpret_cast<const char*>(a) : std::string();
      out.push_back(ev);
    }
    sqlite3_finalize(stmt);
    return out;
  }
private:
  sqlite3 *db;
};

IConfigStore* create_sqlite_config_store(const char* dbpath){
  try{
    return new SQLiteConfigStore(dbpath);
  }catch(...){ return nullptr; }
}

IFeederRepository* create_sqlite_repo(const char* dbpath){
  try{
    return new SQLiteFeederRepo(dbpath);
  }catch(...){ return nullptr; }
}
#else

class SQLiteConfigStore : public IConfigStore {
public:
  SQLiteConfigStore(const char* path) {}
  virtual ~SQLiteConfigStore() {}
  virtual bool save(const DeviceConfig &cfg) override { return false; }
  virtual bool load(DeviceConfig &out) override { return false; }
};

class SQLiteFeederRepo : public IFeederRepository {
public:
  SQLiteFeederRepo(const char* path) {}
  virtual ~SQLiteFeederRepo() {}
  virtual void addEvent(const FeedEvent &ev) override {}
  virtual std::vector<FeedEvent> recent(int limit) override { return {}; }
};

IConfigStore* create_sqlite_config_store(const char* dbpath){ return nullptr; }
IFeederRepository* create_sqlite_repo(const char* dbpath){ return nullptr; }
#endif
