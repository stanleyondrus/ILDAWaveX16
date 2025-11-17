#ifndef RENDERED_H
#define RENDERED_H

#include <Arduino.h>
#include <DAC80508.h>
#include <ILDA.h>
#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <PointRingBuffer.h>
#include "esp_task_wdt.h"

#define PIN_BTN 0
#define PIN_LED 7
#define PIN_Shutter 8
#define PIN_MISO 9
#define PIN_CS 10
#define PIN_MOSI 11
#define PIN_SCK 12

#define POINTS_PER_BUFFER 1024

class Renderer {
  public:
    void shutterLow();
    void shutterHigh();

    void buffer_add_point(const Point& p);
    void buffer_add_points(const Point* p, uint16_t num);
    void buffer_clear_points();

    void start();
    void reset();
    void stop();

    void sd_stop();
    void sd_start(File file);
 
    void change_freq(uint32_t val);
    void change_brightness(uint8_t val);

    void begin();

    static void SDTask(void *pvParameters);
    static void DACTask(void *pvParameters);

    static SemaphoreHandle_t dacSem;
  
    uint8_t rendererRunning = 0;
    uint8_t sdRunning = 0;

    uint32_t timer_val = 10; // T[us] = 1000000 / f [Hz]
    uint8_t brightness = 100; // 0-100%

  private:
    spi_device_handle_t spi;
    DAC80508 dac;
    ILDA ilda;
    File ildaFile;
    hw_timer_t* dacTimer = nullptr;
  
};

#endif /* RENDERED_H */