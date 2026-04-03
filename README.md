
 Klangbox:

 Plays MP3s from filesystem if there is motion detected
 
 Hardware: ESP32-S2   lolin_s2_mini board
           SR602      motion sensor
           MAX98357A  digital audio interface 
           SD05CRMA   LiPo charging
           Speaker    8 Ohm

GPIO Pins used:
GPIO_NUM_4:    Motion Sensor
GPIO_NUM_15:   Onboard LED (for simple debugging)
GPIO_NUM_5:    Volume Potentiometer
GPIO_NUM_9:    Batteriespannung
GPIO_NUM_7:    MAX98357A  BLK
GPIO_NUM_11:   MAX98357A  DIN
GPIO_NUM_33:   MAX98357A  LRC

Deep sleep is used while the ULP (Ultra Low Coprozessor) executes a small program written in C.
This program checks periodically the motion sensor. The motion sensor detects a movement and wakes up the main processor.

Framework is ESP-IDF. 
Arduino framework can't be used because the toolchain does not support the ulp risc processor programmed in C.
