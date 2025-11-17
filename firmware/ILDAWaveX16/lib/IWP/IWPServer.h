#ifndef IWPSERVER_H
#define IWPSERVER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <Renderer.h>

#define IW_UDP_PORT 7200

#define IWP_BUFFER_SIZE 1024

#define IW_TYPE_0 0x00 // Turn off
#define IW_TYPE_1 0x01 // Period
#define IW_TYPE_2 0x02 // 16b X/Y + 8b R/G/B
#define IW_TYPE_3 0x03 // 16b X/Y + 16b R/G/B

// TYPE 0 - Turn off
//  0
// +------+
// | 0x00 |
// +------+

// TYPE 1 - 32b Period
//  0     1      2      3      4
// +------+------+------+------+------+
// | 0x01 |           PERIOD          |
// +------+------+------+------+------+

// TYPE 2 - 16b X/Y + 8b R/G/B
//  0     1      2      3      4      5      6      7
// +------+------+------+------+------+------+------+------+
// | 0x02 |      X      |      Y      |   R  |   G  |   B  |
// +------+------+------+------+------+------+------+------+

// TYPE 3 - 16b X/Y + 16b R/G/B
//  0     1      2      3      4      5      6      7      8      9      10     
// +------+------+------+------+------+------+------+------+------+------+------+
// | 0x03 |      X      |      Y      |      R      |      G      |      B      |
// +------+------+------+------+------+------+------+------+------+------+------+

class IWPServer {
  public:
    void begin();
    void stop();
    void setRendererHandle(Renderer* renderer);
    void loop();
    uint16_t iw_period = 1; // [ms]
  private:
    WiFiUDP udp;
    uint8_t udpOpen;
    Renderer* rendererPtr = nullptr;
    uint8_t netRxBuffer[1500];
};

#endif /* IWPSERVER_H */