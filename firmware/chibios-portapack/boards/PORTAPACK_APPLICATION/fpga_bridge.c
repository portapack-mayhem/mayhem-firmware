// This bridge allows calls for functions in their native C context
// HackRF headers here - C for use within the C++ board.cpp context

// Check if PRALINE was passed from CMake
#ifdef PRALINE
  #warning "Building for HackRF_PRO with FPGA."
 
  // Necessary headers for direct programming
  #include "w25q80bv.h"
  #include "spi_bus.h"
  #include "ice40_spi.h"

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

  // Define the pins using positional values (Port, Pin)
  static const gpio_t led1 = { 2, 1 };
  static const gpio_t led2 = { 2, 2 };
  static const gpio_t led3 = { 2, 8 };

  // Local context structure (must match fpga_image.c)
  struct fpga_image_read_ctx {
      uint32_t addr;
      size_t next_block_sz;
      uint8_t init_flag;
      uint8_t buffer[4096 + 2];
  };

  // Local implementation of the callback since the original is static
  static size_t local_fpga_read_block_cb(void* _ctx, uint8_t* out_buffer) {
      struct fpga_image_read_ctx* ctx = (struct fpga_image_read_ctx*)_ctx;
      size_t block_sz = ctx->next_block_sz;

      // First iteration: read first block size
      if (ctx->init_flag == 0) {
          w25q80bv_read(&spi_flash, ctx->addr, 2, ctx->buffer);
          block_sz = ctx->buffer[0] | (ctx->buffer[1] << 8);
          ctx->addr += 2;
          ctx->init_flag = 1;
      }

      // Finish at end marker
      if (block_sz == 0) return 0;

      // Read compressed block and next block size
      w25q80bv_read(&spi_flash, ctx->addr, block_sz + 2, ctx->buffer);
      ctx->addr += block_sz + 2;
      ctx->next_block_sz = ctx->buffer[block_sz] | (ctx->buffer[block_sz + 1] << 8);

      // Decompress block using the LZ4 driver
      return lz4_blk_decompress(ctx->buffer, out_buffer, block_sz);
  }

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

  int fpga_bridge_init(void) {
      // Step 0: MCU reached boardInit
      //gpio_set(&led3); // LED3 ON (P6_12) TX

      pin_setup();
      //gpio_set(&led1); // LED1 ON (P4_1) USB
				   
      si5351c_init(&si5351c_driver);
      gpio_set(&led2); // LED2 ON (P4_2) RX

      //return fpga_image_load(0);
      
      // Prepare for SPI flash access for accessing praline_fpga.bin
      // NOTE: !!! This approach to save space requires flashing your 
      // raw FPGA bitstream to the 1MB offset!!!
      // Use the command: hackrf_spiflash -a 0x100000 -w praline_fpga.bin
      spi_bus_start(spi_flash.bus, &ssp_config_w25q80bv);
      w25q80bv_setup(&spi_flash);

      // Initialize the FPGA target pins and clock
      ice40_spi_target_init(&ice40);
      ssp1_set_mode_ice40();

      // Set up the context to point to your chosen flash address
      struct fpga_image_read_ctx ctx = {
          .addr = 0x100000, // The 1MB mark you chose
          .init_flag = 0    // Forces the callback to read the first block size
      };

      // Call the programmer directly, bypassing fpga_image_load()
      // This uses the callback from fpga_image.c to stream from 0x100000
      const bool success = ice40_spi_syscfg_program(
          &ice40,
          local_fpga_read_block_cb, //Uses locally defined function.
          &ctx
      );

      // Restore SPI mode for the radio components
      ssp1_set_mode_max283x();

      return success ? 0 : -1;
  }
#else
#warning "Building for HackRF_One with CPLD."
#endif
