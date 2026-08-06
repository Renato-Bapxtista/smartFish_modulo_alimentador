#pragma once

#include "ports.h"

IFeederDriver* create_feeder_adapter();
IConfigStore* create_inmemory_config_store();
IFeederRepository* create_inmemory_repo();
// Optional SQLite-backed adapters (call with path to DB file)
IConfigStore* create_sqlite_config_store(const char* dbpath);
IFeederRepository* create_sqlite_repo(const char* dbpath);
