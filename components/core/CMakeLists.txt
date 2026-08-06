idf_component_register(SRCS "core.cpp"
                       "adapters/feeder_adapter.cpp"
                       "adapters/inmemory_adapters.cpp"
                       "adapters/sqlite_adapters.cpp"
                       INCLUDE_DIRS "include" "adapters"
                       REQUIRES feeder_controller)
