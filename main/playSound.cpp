#include "configuration.h"

#if defined(WITH_AUDIO)
#include "esp_log.h"
#include <list>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "playSound.h"
#include "fileList.h"


extern void blinkLED(int count, int dly);

RTC_DATA_ATTR int counter=0;

static FileList *fl = FileList::getInstance();

PlaySound* PlaySound::instance = 0;

PlaySound *PlaySound::getInstance() {
  if (!instance)
  {
      instance = new PlaySound();
  }
  return instance;
}


// Constructor
PlaySound::PlaySound()
  : source(startFilePath, ext),
    player(source, i2s, decoder)
{

}


// Initialize filesystem
void PlaySound::initFS() {
  if ( !LittleFS.begin() ) 
  {
    ESP_LOGI(PLTAG,"LittleFS Mount fehlgeschlagen");
    if ( !LittleFS.format() )
    {
      ESP_LOGI(PLTAG,"Formatierung nicht möglich");
    }
    else
    {
      ESP_LOGI(PLTAG,"Formatierung erfolgreich");
      if ( !LittleFS.begin() )
      {
        ESP_LOGI(PLTAG,"LittleFS Mount trotzdem fehlgeschlagen");
      }
      else 
      {
        ESP_LOGI(PLTAG,"LittleFS Dateisystems erfolgreich gemounted!"); 
      }
    }
  }  
  else
  {
    ESP_LOGI(PLTAG,"LittleFS erfolgreich gemounted!");
    //blinkLED(20,200);
  }
}


// Check Volume, Map Analog Read 0-4095 (0-3.3V) to 0-1.0
float PlaySound::getVolume() {
    float vol = map(analogRead(VOLUME),0,4095,0,1000);
    vol /= 1000;
#if defined(WITH_TST)
    vol = 0.6;
#endif
    return vol==0.0?0.2:vol;
}


// Check Battery, Map Analog Read 0-4095 (0-3.3V) to 0-3.3
float PlaySound::getBatterie() {
    float bat = map(analogRead(BATTERIE),0,4095,0,3300);
    ESP_LOGI(PLTAG,"getBatterie=%f\n",bat);
    bat /= 1000;
#ifdef WITH_TST
    return 2.9;
#endif
    return bat==0.0?3.3:bat;
}


// Set next file to play, counter is set in RTC memory 
void PlaySound::setFileFilter() {
  // Select sound file
  fileFilter[0]='\0';
  if(counter<fl->dirList.size()) {
    std::list<std::tuple<String, String, int>>::iterator it = fl->dirList.begin();
    advance(it, counter);
    strcpy(fileFilter,(get<1>(*it)).c_str());
    if(++counter>=fl->dirList.size()-2)
      counter=0;
  }
  source.setFileFilter(fileFilter);
}


// Plays either next MP3 or Battery MP3 hint if Battery goes low
void PlaySound::doPlaySound() {
    ESP_LOGI(PLTAG,"doPlaySound\n");

    // Analog PINs for Volume and Battery setting
    pinMode(VOLUME,INPUT);
    pinMode(BATTERIE,INPUT);
    
    if(getBatterie() < 3.0) { // if battery less than 3V play Sound "Battery almost empty"
      source.setFileFilter("_Battery*");  
      dly=11000;
    } else {
        setFileFilter();
        dly=0;
    }

    // setup output
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck = MYBCLK;
    cfg.pin_data = DIN;
    cfg.pin_ws = MYLRC;
    cfg.channels = 1;
    i2s.begin(cfg);

    // setup player
    //player.setMetadataCallback(printMetaData);
    //AudioLogger::instance().begin(Serial, AudioLogger::Info);

    player.setVolume(getVolume());

    ESP_LOGI(PLTAG,"Player begin\n");

    // Play sound
    player.begin();
    while (player.isActive()) { 
        player.setVolume(getVolume());
        player.copy(); 
    };
    player.end();

    ESP_LOGI(PLTAG,"Player end, dly=%d\n", dly);

    vTaskDelay(pdMS_TO_TICKS(dly));

}

#endif