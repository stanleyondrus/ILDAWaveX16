#ifndef DAC80508_H
#define DAC80508_H

#include <Arduino.h>
#include <SPI.h>
#include "driver/spi_master.h"
#include <ILDA.h>

#define SPI_CLK_SPEED 50000000

#define REG_NOP 0x00
#define REG_DEVID 0x81 // Read only
#define REG_SYNC 0x02
#define REG_CONFIG 0x03
#define REG_GAIN 0x04
#define REG_TRIGGER 0x05
#define REG_BRDCAST 0x06
#define REG_STATUS 0x07
#define REG_DACx 0x08 // CH1 0x08 -  CH8 0x0F

#define DAC_CH_X 6
#define DAC_CH_Y 7
#define DAC_CH_R 5
#define DAC_CH_G 4
#define DAC_CH_B 3
#define DAC_CH_I 2
#define DAC_CH_U1 1
#define DAC_CH_U2 0

class DAC80508 {
  public:
    void begin(spi_device_handle_t spih, uint8_t cs, uint8_t sck, uint8_t mosi, uint8_t miso);
    void write_register(uint8_t reg, uint8_t b1, uint8_t b2);
    void dac_write(uint8_t channel, uint16_t data);
    void dac_write_point(Point p);
    void dac_write_color(uint16_t r, uint16_t g, uint16_t b);
    void dac_sync( );
  private:
    uint8_t cs_pin = 0;
    spi_device_handle_t spi;
};

#endif /* DAC80508_H */