// This bridge allows calls for functions in their native C context
// HackRF headers here - C for use within the C++ board.cpp context

// Check if PRALINE was passed from CMake
#ifdef PRALINE
  #warning "Building for HackRF_PRO with FPGA."
  
  // LIBOPENCM3 Headers
  #include <libopencm3/lpc43xx/ipc.h>
  #include <libopencm3/lpc43xx/m4/nvic.h>
  #include <libopencm3/lpc43xx/rgu.h>
  #include <libopencm3/lpc43xx/timer.h>

  // HACKRF HEADERS
  #include "si5351c.h"
  #include "fpga.h"
  #include "hackrf_core.h"
  #include "adc.h"
  #include "gpio.h"
  #include "i2c_bus.h"
  #include "i2c_lpc.h"
  #include "platform_detect.h"

  i2c_bus_t i2c_bus0 = {
      .obj = (void*) I2C0_BASE,
      .start = i2c_lpc_start,
      .stop = i2c_lpc_stop,
      .transfer = i2c_lpc_transfer,
  }; 

  si5351c_driver_t si5351c_driver = {
      .bus = &i2c_bus0,
      .i2c_address = 0x60 //SI5351C_I2C_ADDRESS,
  };

  // Define the pins using positional values (Port, Pin)
  static const gpio_t led1 = { 2, 1 };
  static const gpio_t led2 = { 2, 2 };
  static const gpio_t led3 = { 2, 8 };

  int fpga_bridge_init(void) {
      // Step 0: MCU reached boardInit
//      gpio_set(&led3); // LED3 ON (P6_12) TX

      pin_setup();
//      gpio_set(&led1); // LED1 ON (P4_1) USB
				   
      si5351c_init(&si5351c_driver);
      gpio_set(&led2); // LED2 ON (P4_2) RX

      return fpga_image_load(0);
  }
#else
#warning "Building for HackRF_One with CPLD."
#endif
