#pragma once

#define PLTAG "PLAY"

#include <Arduino.h>

#include "Audio.h"
#include "AudioTools.h"
#include "AudioTools/Disk/AudioSourceLittleFS.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"


class PlaySound
{
public:
    static PlaySound* getInstance();

    void initFS(void);
    float getVolume(void); 
    float getBatterie(void);
    void setFileFilter(void);
    void doPlaySound(void);

private:
    PlaySound();
    static PlaySound *instance;


    int dly;
    const char *startFilePath = "/";
    const char *ext = "mp3";
    AudioSourceLittleFS source;
    I2SStream i2s;
    MP3DecoderHelix decoder;
    AudioPlayer player;
    char fileFilter[256];

};