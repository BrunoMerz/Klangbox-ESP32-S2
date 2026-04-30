#define APNAME "Klangbox"
    
// GPIO PINs
#define BEWEGUNG GPIO_NUM_4
#define VOLUME GPIO_NUM_3
#define BATTERIE GPIO_NUM_5

#define MYBCLK 7
#define DIN 11
#define MYLRC 33

// ULP config
#define ULP_WAKEUP_INTERVAL_MS 100

// Klangbox settings
#define LOOPCNTMAX 60000 // Milliseconds

//#define WITH_TST
//#define WITH_BLINK
#define WITH_WIFI
#define WITH_AUDIO