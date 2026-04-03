#define APNAME "Klangbox"

// GPIO PINs
#define BEWEGUNG GPIO_NUM_4
#define VOLUME 2
#define BATTERIE 35


//#define PROTOTYP

#ifdef PROTOTYP
#define BCLK 27
#define DIN 25
#define LRC 26
#else
#define MYBCLK 7
#define DIN 11
#define MYLRC 33
#endif

// ULP config
#define ULP_WAKEUP_INTERVAL_MS 100

// Klangbox settings
#define LOOPCNTMAX 45000 // Milliseconds

#define WITH_TST
//#define WITH_BLINK
#define WITH_WIFI
#define WITH_AUDIO