#include <Arduino.h>
#include "configuration.h"
#include "fileList.h"
#include "esp_log.h"

#include <FS.h>
#include <LittleFS.h>


extern void blinkLED(int count, int dly);

FileList* FileList::instance = 0;

FileList *FileList::getInstance() {
  if (!instance)
  {
      instance = new FileList();
  }
  return instance;
}

FileList::FileList()
{

}


// Sort file list ascending
void FileList::sortList(void) {
  ESP_LOGI(FLTAG,"vor sort");
  dirList.sort([](const records & f, const records & l) {
    if (get<0>(f)[0] != 0x00 || get<0>(l)[0] != 0x00) {
      for (uint8_t i = 0; i < 31; i++) {
          if (tolower(get<0>(f)[i]) < tolower(get<0>(l)[i])) return true;
          else if (tolower(get<0>(f)[i]) > tolower(get<0>(l)[i])) return false;
      }
    }
    return false;
  });
  ESP_LOGI(FLTAG,"nach sort");
}


// Read filesystem an build list of all files
bool FileList::buildList(void) {
  if(dirList.size())
    return true;

  ESP_LOGI(FLTAG,"Read Filesystem");
  usedBytes = LittleFS.usedBytes();
  totalBytes = LittleFS.totalBytes();
  freeBytes = totalBytes - usedBytes;
  File root = LittleFS.open("/");
  File dir = root.openNextFile();
  
  while(dir) {
    if(dir.isDirectory()) {
      
      uint8_t ran {0};
      ESP_LOGI(FLTAG,"buildList <%s>\n",dir.name());
      String fn="/"+String(dir.name());
      File root2 = LittleFS.open(fn);
      File fold = root2.openNextFile();
      while (fold)  {
        ran++;
        dirList.emplace_back(fn, fold.name(), fold.size());
        fold = root2.openNextFile();
      }
      if (!ran) dirList.emplace_back(fn, "", 0);
    }
    else {
      dirList.emplace_back("", dir.name(), dir.size());
    }
    dir = root.openNextFile();
  }
  int count = dirList.size();
  if(count)
    blinkLED(count,1000);
  sortList();

  return true;
}