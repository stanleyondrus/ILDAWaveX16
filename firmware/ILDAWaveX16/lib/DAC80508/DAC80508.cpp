#include "DAC80508.h"

void DAC80508::begin(spi_device_handle_t spih, uint8_t cs, uint8_t sck, uint8_t mosi, uint8_t miso) {
  spi = spih;
  
  spi_bus_config_t buscfg = {};
  buscfg.mosi_io_num = mosi;
  buscfg.miso_io_num = miso;
  buscfg.sclk_io_num = sck;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);

  spi_device_interface_config_t devcfg = {};
  devcfg.clock_speed_hz = SPI_CLK_SPEED;
  devcfg.mode = 1;
  devcfg.spics_io_num = cs;
  devcfg.queue_size = 12;
  devcfg.flags = SPI_DEVICE_NO_DUMMY;
  spi_bus_add_device(SPI3_HOST, &devcfg, &spi);

  write_register(REG_TRIGGER, 0x00, 0b00001010); // SOFT-RESET
  write_register(REG_GAIN, 0x00, 0xFF); // REFDIV-EN 0, BUFF-GAIN 2x
  write_register(REG_SYNC, 0xFF, 0xFF); // BRDCAST-EN, SYNC-EN
}

void DAC80508::write_register(uint8_t reg, uint8_t b1, uint8_t b2) {
  static uint8_t txbuf[4]; // 4 bytes, aligned
  txbuf[0] = reg;
  txbuf[1] = b1;
  txbuf[2] = b2;
  txbuf[3] = 0; // padding, ignored

  spi_transaction_t t = {};
  t.length = 24;       // 24 bits to send
  t.tx_buffer = txbuf; // DMA-safe buffer

  spi_device_queue_trans(spi, &t, portMAX_DELAY);
  spi_transaction_t* rtrans;
  spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
}

void DAC80508::dac_write(uint8_t channel, uint16_t data) {
  static uint8_t txbuf[4]; // 4 bytes, last byte unused
  txbuf[0] = REG_DACx + channel;
  txbuf[1] = (data >> 8) & 0xFF;
  txbuf[2] = data & 0xFF;
  txbuf[3] = 0;

  spi_transaction_t t = {};
  t.length = 24;
  t.tx_buffer = txbuf;
  t.rx_buffer = nullptr;

  spi_device_queue_trans(spi, &t, portMAX_DELAY);
  // spi_transaction_t* rtrans;
  // spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
}

void DAC80508::dac_write_point(Point p) { 
  uint8_t txbuf[6][3];
  txbuf[0][0] = REG_DACx + DAC_CH_X;
  txbuf[0][1] = (p.x >> 8) & 0xFF;
  txbuf[0][2] = p.x & 0xFF;

  txbuf[1][0] = REG_DACx + DAC_CH_Y;
  txbuf[1][1] = (p.y >> 8) & 0xFF;
  txbuf[1][2] = p.y & 0xFF;

  txbuf[2][0] = REG_DACx + DAC_CH_R;
  txbuf[2][1] = (p.r >> 8) & 0xFF;
  txbuf[2][2] = p.r & 0xFF;

  txbuf[3][0] = REG_DACx + DAC_CH_G;
  txbuf[3][1] = (p.g >> 8) & 0xFF;
  txbuf[3][2] = p.g & 0xFF;

  txbuf[4][0] = REG_DACx + DAC_CH_B;
  txbuf[4][1] = (p.b >> 8) & 0xFF;
  txbuf[4][2] = p.b & 0xFF;

  txbuf[5][0] = REG_TRIGGER;
  txbuf[5][1] = 0x00;
  txbuf[5][2] = 0x10;

  spi_transaction_t t[6] = {};
  for (int i = 0; i < 6; i++) {
      t[i].length = 24;
      t[i].tx_buffer = txbuf[i];
      spi_device_queue_trans(spi, &t[i], portMAX_DELAY);
  }

  // spi_transaction_t* rtrans;
  // for (int i = 0; i < 6; i++) {
  //    spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
  // }
}

void DAC80508::dac_write_color(uint16_t r, uint16_t g, uint16_t b) { 
  uint8_t txbuf[4][3];

  txbuf[0][0] = REG_DACx + DAC_CH_R;
  txbuf[0][1] = (r >> 8) & 0xFF;
  txbuf[0][2] = r & 0xFF;

  txbuf[1][0] = REG_DACx + DAC_CH_G;
  txbuf[1][1] = (g >> 8) & 0xFF;
  txbuf[1][2] = g & 0xFF;

  txbuf[2][0] = REG_DACx + DAC_CH_B;
  txbuf[2][1] = (b >> 8) & 0xFF;
  txbuf[2][2] = b & 0xFF;

  txbuf[3][0] = REG_TRIGGER;
  txbuf[3][1] = 0x00;
  txbuf[3][2] = 0x10;

  spi_transaction_t t[4] = {};
  for (int i = 0; i < 4; i++) {
      t[i].length = 24;
      t[i].tx_buffer = txbuf[i];
      spi_device_queue_trans(spi, &t[i], portMAX_DELAY);
  }

  // spi_transaction_t* rtrans;
  // for (int i = 0; i < 6; i++) {
  //    spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
  // }
}

void DAC80508::dac_sync() {
  static uint8_t txbuf[3] = {REG_TRIGGER, 0x00, 0x10};

  spi_transaction_t t = {};
  t.length = 24;          // 24 bits
  t.tx_buffer = txbuf;
  t.rx_buffer = nullptr;

  spi_device_queue_trans(spi, &t, portMAX_DELAY);
  // spi_transaction_t* rtrans;
  // spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
}