#include "IDNServer.h"

void IDNServer::begin() {
  udp.begin(IDNVAL_HELLO_UDP_PORT);
  Serial.printf("IDN UDP Port: %i\n", IDNVAL_HELLO_UDP_PORT);
  udpOpen = 1;
}

void IDNServer::stop() {
  udp.stop();
  udpOpen = 0;
}

void IDNServer::setRendererHandle(Renderer* renderer) {
  rendererPtr = renderer;
}

void IDNServer::loop() {
  if (udpOpen == 0 || udp.parsePacket() == 0) return;

  int len = udp.read(netRxBuffer, sizeof(netRxBuffer));
  // if (len != 4) return; // IDNHDR_PACKET
  if (len == 0) return; // didn't read anything
  
  IDNHDR_PACKET *recvPacketHdr = (IDNHDR_PACKET *)(netRxBuffer);
  unsigned short recvSequence = ntohs(recvPacketHdr->sequence);
  uint8_t cmd = recvPacketHdr->command;

  // Serial.print("RX: ");
  // for (int i=0; i<len; i++) {
  //   Serial.printf("%02X ", netRxBuffer[i]);
  // }
  // Serial.println();

  switch (cmd) {

    case IDNCMD_SCAN_REQUEST: {
      unsigned sendLen = sizeof(IDNHDR_PACKET) + sizeof(IDNHDR_SCAN_RESPONSE);

      // IDN-Hello packet header
      IDNHDR_PACKET* sendPacketHdr = (IDNHDR_PACKET*)(netTxBuffer);
      sendPacketHdr->command = IDNCMD_SCAN_RESPONSE;
      sendPacketHdr->flags = recvPacketHdr->flags & IDNMSK_PKTFLAGS_GROUP;
      sendPacketHdr->sequence = recvPacketHdr->sequence;

      // Scan response header
      IDNHDR_SCAN_RESPONSE* scanRspHdr = (IDNHDR_SCAN_RESPONSE*)&sendPacketHdr[1];
      memset(scanRspHdr, 0, sizeof(IDNHDR_SCAN_RESPONSE));
      scanRspHdr->structSize = sizeof(IDNHDR_SCAN_RESPONSE);
      scanRspHdr->protocolVersion = (unsigned char)((0 << 4) | 1);

      // Populate status
      scanRspHdr->status = 0;
      scanRspHdr->status |= IDNFLG_SCAN_STATUS_REALTIME;  // Offers IDN-RT reatime streaming
      // if (checkExcluded(recvPacketHdr->flags)) scanRspHdr->status |= IDNFLG_SCAN_STATUS_EXCLUDED;
      // FIXME: unit issues -> IDNFLG_SCAN_STATUS_OFFLINE;

      // Populate unitID and host name
      scanRspHdr->unitID[0] = 0x07; // Unit ID Length
      scanRspHdr->unitID[1] = 0x01; // Unit ID Type: EUI-48 MAC
      esp_read_mac(&scanRspHdr->unitID[2], ESP_MAC_WIFI_STA); // Unit ID: Copy MAC
      sprintf((char*)scanRspHdr->hostName, IDN_HOSTNAME); // copy host name

      // Send response back to requester
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.write(netTxBuffer, sendLen);
      udp.endPacket();
      // Serial.println(udp.endPacket());
      // Serial.printf("Send to IP: %s\n", udp.remoteIP().toString().c_str());
      break;
    }

    case IDNCMD_SERVICEMAP_REQUEST: {
      unsigned sendLen = sizeof(IDNHDR_PACKET) + sizeof(IDNHDR_SERVICEMAP_RESPONSE);
      
      // IDN-Hello packet header
      IDNHDR_PACKET *sendPacketHdr = (IDNHDR_PACKET *)(netTxBuffer);
      sendPacketHdr->command = IDNCMD_SERVICEMAP_RESPONSE;
      sendPacketHdr->flags = recvPacketHdr->flags & IDNMSK_PKTFLAGS_GROUP;
      sendPacketHdr->sequence = recvPacketHdr->sequence;

      // Service map header setup
      IDNHDR_SERVICEMAP_RESPONSE *mapRspHdr = (IDNHDR_SERVICEMAP_RESPONSE *)&sendPacketHdr[1];
      memset(mapRspHdr, 0, sizeof(IDNHDR_SERVICEMAP_RESPONSE));
      mapRspHdr->structSize = sizeof(IDNHDR_SERVICEMAP_RESPONSE);
      mapRspHdr->entrySize = sizeof(IDNHDR_SERVICEMAP_ENTRY);

      // ---------------------------------------------

      // Build tables
      unsigned relayCount = 0, serviceCount = 0;
      IDNHDR_SERVICEMAP_ENTRY relayTable[0xFF];
      IDNHDR_SERVICEMAP_ENTRY serviceTable[0xFF];
      
      uint8_t flags = 0;
      flags |= IDNFLG_MAPENTRY_DEFAULT_SERVICE;
      
      IDNHDR_SERVICEMAP_ENTRY *mapEntry = &serviceTable[serviceCount];
      mapEntry->serviceID = 0x01;
      mapEntry->serviceType = IDNVAL_SMOD_LPGRF_CONTINUOUS;
      mapEntry->flags = flags;
      mapEntry->relayNumber = 0;
      memset(mapEntry->name, 0, sizeof(mapEntry->name));
      sprintf((char*)mapEntry->name, IDN_SERVICE_NAME); // copy service name

      serviceCount++;

      // for(auto &service : services) {

      //     uint8_t flags = 0;
      //     if(service->isDefaultService()) flags |= IDNFLG_MAPENTRY_DEFAULT_SERVICE;

      //     // Populate the entry. Note: Name field not terminated, padded with '\0'
      //     IDNHDR_SERVICEMAP_ENTRY *mapEntry = &serviceTable[serviceCount];
      //     mapEntry->serviceID = service->getServiceID();
      //     mapEntry->serviceType = service->getServiceType();
      //     mapEntry->flags = flags;
      //     mapEntry->relayNumber = 0;
      //     memset(mapEntry->name, 0, sizeof(mapEntry->name));
      //     service->copyServiceName((char *)(mapEntry->name), sizeof(mapEntry->name));

      //     // Next entry
      //     serviceCount++;
      // }

      // Add memory needed for the tables
      sendLen += relayCount * sizeof(IDNHDR_SERVICEMAP_ENTRY);
      sendLen += serviceCount * sizeof(IDNHDR_SERVICEMAP_ENTRY);

      // Copy tables
      IDNHDR_SERVICEMAP_ENTRY *mapEntry1 = (IDNHDR_SERVICEMAP_ENTRY *)&mapRspHdr[1];
      memcpy(mapEntry1, relayTable, relayCount * sizeof(IDNHDR_SERVICEMAP_ENTRY));
      mapEntry = &mapEntry1[relayCount];
      memcpy(mapEntry1, serviceTable, serviceCount * sizeof(IDNHDR_SERVICEMAP_ENTRY));

      // Populate header entry counts
      mapRspHdr->relayEntryCount = (uint8_t)relayCount;
      mapRspHdr->serviceEntryCount = (uint8_t)serviceCount;

      // ---------------------------------------------

      // Send response back to requester
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.write(netTxBuffer, sendLen);
      udp.endPacket();
      // Serial.printf("Send to IP: %s\n", udp.remoteIP().toString().c_str());
      break;
    }
    case IDNCMD_RT_CNLMSG: {     
      IDNHDR_CHANNEL_MESSAGE* recvChannelMsg = (IDNHDR_CHANNEL_MESSAGE*)&recvPacketHdr[1];
      uint16_t channelMsgTotalSize = ntohs(recvChannelMsg->totalSize);
      uint16_t channelMsgContentID = ntohs(recvChannelMsg->contentID);
      uint32_t channelMsgTimestamp = ntohl(recvChannelMsg->timestamp);
      // Serial.printf("%u\t%04X\t%u\n", channelMsgTotalSize, channelMsgContentID, channelMsgTimestamp);
      
      if ((channelMsgContentID & 0x01) == 0) break; // not a wave message

      if (channelMsgContentID & 0x4000) {
        recvChannelMsg = (IDNHDR_CHANNEL_MESSAGE*)((char*)recvChannelMsg + 20); // skip 20 bytes configuration
        channelMsgTotalSize -= 20;
      }

      IDNHDR_SAMPLE_CHUNK* sampleChunkHdr = (IDNHDR_SAMPLE_CHUNK*)&recvChannelMsg[1];

      byte* data = (byte*)&sampleChunkHdr[1];

      uint16_t samples = (channelMsgTotalSize - 12) / 8;
      
      Point p[512];
      
      for (int a = 0; a < samples; a++) {
        int offset = a * 8;

        p[a].x = (data[offset] << 8) | data[offset + 1];
        p[a].y = (data[offset + 2] << 8) | data[offset + 3];
        p[a].r = data[offset + 4];
        p[a].g = data[offset + 5];
        p[a].b = data[offset + 6]; 
        // uint8_t i = data[offset + 7];

        p[a].x = p[a].x + 0x8000;
        p[a].y = -p[a].y + 0x8000;   

        p[a].r = (p[a].r << 8) | p[a].r;
        p[a].g = (p[a].g << 8) | p[a].g;
        p[a].b = (p[a].b << 8) | p[a].b;
      }

      rendererPtr->buffer_add_points(p, samples);
    }

  }

}