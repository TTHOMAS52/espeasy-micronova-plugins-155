  struct Code_Ram_Rom_Read_Write{ int Code_Read_RAM;
                int Code_Read_ROM;
                    int Code_Write_RAM;
                    int Code_Write_ROM;
   };





#ifndef PLUGINSTRUCTS_P155_DATA_STRUCT_H
#define PLUGINSTRUCTS_P155_DATA_STRUCT_H

#include "../../_Plugin_Helper.h"
#ifdef USES_P155


struct P155_data_struct : public PluginTaskData_base {
  /*********************************************************************************************\
  * Task data struct to simplify taking measurements of upto 4 Dallas DS18b20 (or compatible)
  * temperature sensors at once.
  *
  * Limitations:
  * - Use the same GPIO pin
  * - Use the same resolution for all sensors of the same task
  * - Max 4 sensors queried at the same time
  *
  * The limit of 4 sensors is determined by the way the settings are stored and it
  * is a practical limit to make sure we don't spend too much time in a single call.
  *
  * Using the same resolution is to make it (a lot) simpler as all sensors then need the
  * same measurement time.
  *
  * If those limitations are not desired, use multiple tasks.
  \*********************************************************************************************/

  // @param pin  The GPIO pin used to communicate to the Dallas sensors in this task
  // @param res  The resolution of the Dallas sensor(s) used in this task
  P155_data_struct(taskIndex_t taskIndex,
                   int8_t      pin_rx,
                   int8_t      pin_tx,
                   int8_t     enable_pin_rx
  );


  virtual ~P155_data_struct() = default;

  void init();

  



  // Read temperature from the sensor at given index.
  // May return false if the sensor is not present or address is zero.
  
  int8_t get_gpio_rx() const {
    return _gpio_rx;
  }

  int8_t get_gpio_tx() const {
    return _gpio_tx;
  }
  int8_t gpio_enable_rx() const {
    return _gpio_enable_rx;
  }


  
private:

  // Do not set the _timer to 0, since it may cause issues
  // if this object is created (settings edited or task enabled)
  // while the node is up some time between 24.9 and 49.7 days.
 
  taskIndex_t       _taskIndex;
  int8_t            _gpio_rx;
  int8_t            _gpio_tx;
  int8_t            _gpio_enable_rx;
};

#endif // ifdef USES_P155
#endif // ifndef PLUGINSTRUCTS_P155_DATA_STRUCT_H
