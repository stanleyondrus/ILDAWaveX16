#include "Renderer.h"

static PointRingBuffer pointBuffer;

SemaphoreHandle_t Renderer::dacSem = nullptr;

void Renderer::shutterLow() { GPIO.out_w1tc = (1 << PIN_Shutter); }
void Renderer::shutterHigh() { GPIO.out_w1ts = (1 << PIN_Shutter); }

void Renderer::buffer_add_point(const Point& p) { pointBuffer.addPoint(p); }
void Renderer::buffer_add_points(const Point* p, uint16_t num) { pointBuffer.addPoints(p, num); }
void Renderer::buffer_clear_points() { pointBuffer.clear(); }

void Renderer::start() {
  timer_val = 1000000 / 100000;
  timerAlarmWrite(dacTimer, timer_val, true);
  timerAlarmEnable(dacTimer);
  rendererRunning = 1;
}

void Renderer::reset() {
  Point p = { 0, 0, 0, 0, 0 };
  dac.dac_write_point(p);
  shutterLow();
}

void Renderer::stop() {
  timerAlarmDisable(dacTimer);
  timerWrite(dacTimer, 0);
  rendererRunning = 0;
  reset();
  buffer_clear_points();
}

void Renderer::sd_stop() {
  sdRunning = 0;
  if (ildaFile) ildaFile.close();
}

void Renderer::sd_start(File file) {
  if (ilda.readHeader(file)) { sd_stop(); return; }
  ildaFile = file;
  sdRunning = 1;
}

void Renderer::change_freq(uint32_t val) {
  if (val < 10) return;
  timer_val = val;
  timerAlarmWrite(dacTimer, timer_val, true);
}

void Renderer::change_brightness(uint8_t val) { if (val <= 100) brightness = val; }

void IRAM_ATTR timerISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(Renderer::dacSem, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

void Renderer::SDTask(void* pvParameters) {
  Renderer* self = static_cast<Renderer*>(pvParameters);
  while (true) {
    if (!self->sdRunning) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }
    Point p[512];
    int pointsRead = self->ilda.readILDAChunk(p, 512);
    if (pointsRead <= 0) { vTaskDelay(pdMS_TO_TICKS(1)); continue; }
    while (!pointBuffer.canItFit(pointsRead)) { vTaskDelay(pdMS_TO_TICKS(1)); }
    self->buffer_add_points(p, pointsRead);
  }
}

void Renderer::DACTask(void* pvParameters) {
  Renderer* self = static_cast<Renderer*>(pvParameters);
  while (true) {
    if (!self->rendererRunning) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }
    Point points[512];
    uint16_t n = pointBuffer.getPoints(points, 512);
    if (n == 0) { vTaskDelay(pdMS_TO_TICKS(1)); self->dac.dac_write_color(0, 0, 0); continue; }
    for (uint16_t i = 0; i < n; i++) {
      Point& p = points[i];
      if (self->brightness < 100) {
        p.r = (p.r * self->brightness) / 100;
        p.g = (p.g * self->brightness) / 100;
        p.b = (p.b * self->brightness) / 100;
      }
      if (xSemaphoreTake(dacSem, portMAX_DELAY) == pdTRUE) self->dac.dac_write_point(p);
    }
  }
}

void Renderer::begin() {
  pinMode(PIN_Shutter, OUTPUT);
  shutterLow();
  dac.begin(spi, PIN_CS, PIN_SCK, PIN_MOSI, PIN_MISO);

  dacSem = xSemaphoreCreateBinary();

  dacTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(dacTimer, &timerISR, true);

  xTaskCreatePinnedToCore(SDTask, "SDTask", 8192, this, 2, NULL, 0);
  xTaskCreatePinnedToCore(DACTask, "DACTask", 8192, this, 2, NULL, 1);
}