#include "ILDA.h"

uint8_t ILDA::readHeader(File file) {
  if (!file) return 1;

  memset(&ildaStream, 0, sizeof(ILDA_Stream));

  size_t bytesRead = file.read((uint8_t*)&ildaStream.header, sizeof(ILDA_Header_t));
  if (bytesRead != sizeof(ILDA_Header_t)) return 1;

  ildaStream.header.records = ntohs(ildaStream.header.records);
  ildaStream.header.frame_number = ntohs(ildaStream.header.frame_number);
  ildaStream.header.total_frames = ntohs(ildaStream.header.total_frames) +1;

  if (strncmp(ildaStream.header.ilda, "ILDA", 4) != 0) { Serial.println("Invalid ILDA file"); return 1; }
  if (ildaStream.header.format != 0 && ildaStream.header.format != 1 && ildaStream.header.format != 2 && ildaStream.header.format != 4 && ildaStream.header.format != 5) { Serial.printf("Incorrect format code: %d\n", ildaStream.header.format); return 1; }

  ildaStream.data_offset = file.position();  // first record of the first frame
  ildaStream.next_offset = ildaStream.data_offset; // next chunk read starts here
  ildaStream.current_frame_idx = 0;
  ildaStream.current_record_idx = 0;

  const uint8_t bytesPerRecordMap[6] = {8, 6, 3, 0, 10, 8};
  ildaStream.bytes_per_record = (ildaStream.header.format < 6) ? bytesPerRecordMap[ildaStream.header.format] : 0;

  ildaStream.file = file;

  Serial.printf("ILDA Stream ready: format=%d, frames=%d, records/frame=%d, data offset=%d, next offset=%d, bytes/record=%d\n", ildaStream.header.format, ildaStream.header.total_frames, ildaStream.header.records, ildaStream.data_offset, ildaStream.next_offset, ildaStream.bytes_per_record);

  return 0;
}

int ILDA::readILDAChunk(Point* buffer, uint16_t maxPoints) {
  if (!ildaStream.file || ildaStream.bytes_per_record == 0) return 0;

  uint16_t pointsRead = 0;
  File& f = ildaStream.file;

  while (pointsRead < maxPoints) {

    // No records left in this frame - load the next header
    if (ildaStream.current_record_idx >= ildaStream.header.records) {
      ILDA_Header_t nextHeader;
      size_t bytes = f.read((uint8_t*)&nextHeader, sizeof(ILDA_Header_t));
      if (bytes != sizeof(ILDA_Header_t)) {
        // EOF - rewind
        f.seek(0);
        f.read((uint8_t*)&ildaStream.header, sizeof(ILDA_Header_t));
        ildaStream.header.records = ntohs(ildaStream.header.records);
        ildaStream.header.total_frames = ntohs(ildaStream.header.total_frames);
        ildaStream.current_frame_idx = 0;
        ildaStream.current_record_idx = 0;
        ildaStream.next_offset = f.position();
        continue;
      }

      nextHeader.records = ntohs(nextHeader.records);
      nextHeader.total_frames = ntohs(nextHeader.total_frames);

      if (nextHeader.records == 0) {
        // Terminator reached - restart from beginning
        f.seek(0);
        f.read((uint8_t*)&ildaStream.header, sizeof(ILDA_Header_t));
        ildaStream.header.records = ntohs(ildaStream.header.records);
        ildaStream.header.total_frames = ntohs(ildaStream.header.total_frames);
        ildaStream.current_frame_idx = 0;
        ildaStream.current_record_idx = 0;
        ildaStream.next_offset = f.position();
        continue;
      }

      // Valid new frame
      ildaStream.header = nextHeader;
      ildaStream.current_frame_idx++;
      ildaStream.current_record_idx = 0;
      ildaStream.next_offset = f.position();
      continue;
    }

    if (f.position() != ildaStream.next_offset) f.seek(ildaStream.next_offset); // Read single record

    uint8_t temp[10];
    size_t bytes = f.read(temp, ildaStream.bytes_per_record);
    if (bytes != ildaStream.bytes_per_record) {
      // Error - restart stream
      f.seek(0);
      f.read((uint8_t*)&ildaStream.header, sizeof(ILDA_Header_t));
      ildaStream.header.records = ntohs(ildaStream.header.records);
      ildaStream.header.total_frames = ntohs(ildaStream.header.total_frames);
      ildaStream.current_frame_idx = 0;
      ildaStream.current_record_idx = 0;
      ildaStream.next_offset = f.position();
      continue;
    }

    ILDA_Record_t rec = {};
    uint8_t buf_offset = 0;

    if (ildaStream.header.format == 2) {
      ilda_palette[ildaStream.current_record_idx][2] = temp[buf_offset++];
      ilda_palette[ildaStream.current_record_idx][1] = temp[buf_offset++];
      ilda_palette[ildaStream.current_record_idx][0] = temp[buf_offset++];
    }
    else {
      memcpy(&rec.x, &temp[buf_offset], sizeof(int16_t)); buf_offset += 2;
      memcpy(&rec.y, &temp[buf_offset], sizeof(int16_t)); buf_offset += 2;
      if (ildaStream.header.format == 0 || ildaStream.header.format == 4) memcpy(&rec.z, &temp[buf_offset], sizeof(int16_t)); buf_offset += 2;
      rec.status_code = temp[buf_offset++];
      if (ildaStream.header.format == 0 || ildaStream.header.format == 1) rec.color_index = temp[buf_offset++];
      else if (ildaStream.header.format == 4 || ildaStream.header.format == 5) {
        rec.blue = temp[buf_offset++];
        rec.green = temp[buf_offset++];
        rec.red = temp[buf_offset++];
      }
      rec.x = ntohs(rec.x);
      rec.y = ntohs(rec.y);
      rec.z = ntohs(rec.z);
    }

    // Convert to Point
    Point& p = buffer[pointsRead];
    p.x = rec.x + 0x8000;
    p.y = -rec.y + 0x8000;

    if ((rec.status_code & 0b01000000) != 0) p.r = p.g = p.b = 0;
    else {
      if (ildaStream.header.format == 0 || ildaStream.header.format == 1) {
        p.r = (ilda_palette[rec.color_index][0] << 8) | ilda_palette[rec.color_index][0];
        p.g = (ilda_palette[rec.color_index][1] << 8) | ilda_palette[rec.color_index][1];
        p.b = (ilda_palette[rec.color_index][2] << 8) | ilda_palette[rec.color_index][2];
      }
      else if (ildaStream.header.format == 4 || ildaStream.header.format == 5) {
        p.r = (rec.red << 8) | rec.red;
        p.g = (rec.green << 8) | rec.green;
        p.b = (rec.blue << 8) | rec.blue;
      }
    }

    pointsRead++;
    ildaStream.current_record_idx++;
    ildaStream.next_offset = f.position();
  }

  return pointsRead;
}