#include "IWPServer.h"

void IWPServer::begin() {
  udp.begin(IW_UDP_PORT);
  Serial.printf("IWP UDP Port: %i\n", IW_UDP_PORT);
  udpOpen = 1;
}

void IWPServer::stop() {
  udp.stop();
  udpOpen = 0;
}

void IWPServer::setRendererHandle(Renderer* renderer) { rendererPtr = renderer; }

void IWPServer::loop() {
  if (udpOpen == 0 || udp.parsePacket() == 0) return;

  int len = udp.read(netRxBuffer, sizeof(netRxBuffer));
  if (len == 0) return;

  Point points[512];
  int pointCount = 0;
  
  int offset = 0;
  while (offset < len) {
    
    if (netRxBuffer[offset] == IW_TYPE_0) {
      Point p = {0};
      if (pointCount < IWP_BUFFER_SIZE) points[pointCount++] = p;
      rendererPtr->buffer_clear_points();
      offset++;
    }
    else if (netRxBuffer[offset] == IW_TYPE_1) {
      if (offset + 5 > len) break;
      uint32_t value = ((uint32_t)netRxBuffer[offset + 1] << 24) |
        ((uint32_t)netRxBuffer[offset + 2] << 16) |
        ((uint32_t)netRxBuffer[offset + 3] << 8) |
        ((uint32_t)netRxBuffer[offset + 4]);
      rendererPtr->change_freq(value);
      offset += 5;
    }
    else if (netRxBuffer[offset] == IW_TYPE_2) {
      if (offset + 8 > len) break;
      Point p = {0};
      p.x = (netRxBuffer[offset + 1] << 8) | netRxBuffer[offset + 2];
      p.y = (netRxBuffer[offset + 3] << 8) | netRxBuffer[offset + 4];
      p.r = netRxBuffer[offset + 5];
      p.g = netRxBuffer[offset + 6];
      p.b = netRxBuffer[offset + 7]; 
      p.r = map(p.r, 0, 0xFF, 0, 0xFFFF);
      p.g = map(p.g, 0, 0xFF, 0, 0xFFFF);
      p.b = map(p.b, 0, 0xFF, 0, 0xFFFF);
      if (pointCount < IWP_BUFFER_SIZE) points[pointCount++] = p;
      offset += 8;
    }
    else if (netRxBuffer[offset] == IW_TYPE_3) {
      if (offset + 11 > len) break;
      Point p = {0};
      p.x = (netRxBuffer[offset + 1] << 8) | netRxBuffer[offset + 2];
      p.y = (netRxBuffer[offset + 3] << 8) | netRxBuffer[offset + 4];
      p.r = (netRxBuffer[offset + 5] << 8) | netRxBuffer[offset + 6];
      p.g = (netRxBuffer[offset + 7] << 8) | netRxBuffer[offset + 8];
      p.b = (netRxBuffer[offset + 9] << 8) | netRxBuffer[offset + 10];
      if (pointCount < IWP_BUFFER_SIZE) points[pointCount++] = p;
      offset += 11;
    }
    else break;
  }
   if (pointCount > 0) rendererPtr->buffer_add_points(points, pointCount);
}