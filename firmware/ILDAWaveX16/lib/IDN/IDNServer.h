#ifndef IDNSERVER_H
#define IDNSERVER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <Renderer.h>
#include "idn.h"
#include "idn-stream.h"
#include "idn-hello.h"

#define IDN_HOSTNAME "IldaWaveX16"
#define IDN_SERVICE_NAME "IDNService"

// #define DICT_X 0x4200
// #define DICT_Y 0x4210
// #define DICT_PREC_16b 0x4010
// #define DICT_RED_638 0x527E // 638 nm
// #define DICT_GREEN_532 0x5214 // 532 nm
// #define DICT_BLUE_460 0x51CC // 460 nm

class IDNServer {
  public:
    void begin();
    void stop();
    void setRendererHandle(Renderer* renderer);
    void loop();
  private:
    WiFiUDP udp;
    uint8_t udpOpen;
    Renderer* rendererPtr = nullptr;
    uint8_t netRxBuffer[1500];
    uint8_t netTxBuffer[256];
};

#endif /* IDNSERVER_H */