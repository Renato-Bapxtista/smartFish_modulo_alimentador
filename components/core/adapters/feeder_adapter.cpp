#include "ports.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "feeder_controller.h"
#include <stdio.h>

class FeederAdapter : public IFeederDriver {
public:
  virtual bool dispense_ms(int ms) override{
    // Start motor
    feeder_toggle();
    // Wait requested ms
    vTaskDelay(pdMS_TO_TICKS(ms));
    // Stop motor
    feeder_stop();
    return true;
  }
};

// Factory function for simple adapter
IFeederDriver* create_feeder_adapter(){
  static FeederAdapter inst;
  return &inst;
}
