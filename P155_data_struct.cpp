#include "../PluginStructs/P155_data_struct.h"
#include "../../_Plugin_Helper.h"

#ifdef USES_P155

P155_data_struct::P155_data_struct(
  taskIndex_t taskIndex,
  int8_t      pin_rx,
  int8_t      pin_tx,
  int8_t      enable_pin_rx) 
{  
 
}


void P155_data_struct::init() {
  // Explicitly set the pinMode using the "slow" pinMode function
  // This way we know for sure the state of any pull-up or -down resistor is known.
  pinMode(_gpio_rx, INPUT);
  if (_gpio_rx != _gpio_tx) { pinMode(_gpio_tx, OUTPUT);}
  if ((_gpio_rx != _gpio_tx) && (_gpio_rx != _gpio_enable_rx) && (_gpio_tx != _gpio_enable_rx)) {
    pinMode(_gpio_enable_rx, OUTPUT);
  }


}




#endif // ifdef USES_P155
