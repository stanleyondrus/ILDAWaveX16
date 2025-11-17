#include "SDCard.h"

SPIClass SDSPI(FSPI);

void SDCard::begin() {
  SDSPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
}

void SDCard::mount() {
  if (!SD.begin(PIN_SD_CS, SDSPI)) {
    Serial.println("Card Mount Failed");
    return;
  }
  return;
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  }
  else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

  // listDir(SD, "/", 0);
  // readFile(SD, "/ilda.ild");
}

void SDCard::list() {
  const char* dirname = "/";

  Serial.printf("Listing directory: %s\n", dirname);

  File root = SD.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    }
    else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void SDCard::listFiles(vector<String>& list, const char* path) {
  File root = SD.open(path);
  if (!root) return;
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) listFiles(list, file.path());
    else if (strstr(file.path(), ".ild") || strstr(file.path(), ".ILD")) list.push_back(file.path());    
    file.close();
    file = root.openNextFile();
  }
}

String listFilesRecursive(const char *dirname, int depth) {
  String rows;
  File dir = SD.open(dirname);
  if (!dir || !dir.isDirectory()) return "";

  while (true) {
    File file = dir.openNextFile();
    if (!file) break;

    String fullPath = String(dirname);
    if (!fullPath.endsWith("/")) fullPath += "/";
    fullPath += file.name();
    
    if (file.isDirectory()) {
      rows += listFilesRecursive(fullPath.c_str(), depth + 1);
    } else if (strstr(file.path(), ".ild") || strstr(file.path(), ".ILD")) {
      rows += "<tr data-filename='" + fullPath + "'>";
      rows += "<td>" + fullPath + "</td>";
      rows += "<td>" + String(file.size()) + " bytes</td>";
      rows += "</tr>";
    }
    file.close();
  }
  dir.close();
  return rows;
}

String SDCard::generateFileRows() {
  return listFilesRecursive("/", 0);
}

void SDCard::read(const char* path) {
  Serial.printf("Reading file: %s\n", path);

  File file = SD.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

File SDCard::getFile(const char* path) {
  return SD.open(path);
}