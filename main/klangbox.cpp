/*
 * Klangbox:
 *
 * plays MP3s from filesystem if there is motion detected
 * 
 * Hardware: ESP32-S2   lolin_s2_mini board
 *           SR602      motion sensor
 *           MAX98357A  digital audio interface 
 *           SD05CRMA   LiPo charging
 *           Speaker    8 Ohm
 *           see        https://bmerz.de/elektronik/
 */


#include <stdio.h>
#include "esp_sleep.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc_periph.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "ulp_riscv.h"
#include "ulp_main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "configuration.h"
#include "fileList.h"
#include "playSound.h"
#include "handleWebserver.h"


extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

static void init_ulp_program(void);

// Blink function for simple debugging
#if defined(WITH_BLINK)
void blinkLED(int anzahl, int dauer) {
    gpio_reset_pin(GPIO_NUM_15);
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    
    for (int i = 0; i < anzahl; i++) {
    
        gpio_set_level(GPIO_NUM_15, 1); // LED an
        vTaskDelay(pdMS_TO_TICKS(dauer));

        gpio_set_level(GPIO_NUM_15, 0); // LED aus
        vTaskDelay(pdMS_TO_TICKS(dauer));
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
}
#else
void blinkLED(int anzahl, int dauer) {
}
#endif


extern "C" void app_main(void)
{
    // Wait for UART
    vTaskDelay(pdMS_TO_TICKS(200));

    // Get singleton objekts of necessary classes
    FileList *fl = FileList::getInstance();
    PlaySound *ps = PlaySound::getInstance();
    HandleWebserver *hw = HandleWebserver::getInstance();

    blinkLED(2,300);

    // Init Filesystem an build a List of Files
    ps->initFS();
    fl->buildList();

    /* Initialize selected GPIO as RTC IO, enable input, disable pullup and pulldown */
    rtc_gpio_init(GPIO_NUM_4);
    rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(GPIO_NUM_4);
    rtc_gpio_pullup_dis(GPIO_NUM_4);
    rtc_gpio_hold_en(GPIO_NUM_4);

    uint32_t causes = esp_sleep_get_wakeup_causes();
    /* not a wakeup from ULP, load the firmware */
    if (!(causes & BIT(ESP_SLEEP_WAKEUP_ULP))) {
        printf("Not a ULP-RISC-V wakeup, initializing it! \n");
        init_ulp_program();
        hw->handleWeb();
    }

    /* ULP Risc-V read and detected a change in GPIO_0, prints */
    if (causes & BIT(ESP_SLEEP_WAKEUP_ULP)) {
        printf("ULP-RISC-V woke up the main CPU! \n");
        printf("ULP-RISC-V read changes in GPIO_0 current is: %s \n",
            (bool)(ulp_gpio_level_previous == 0) ? "Low" : "High" );
        //gpio_set_level(GPIO_NUM_15, 1); // LED an
        ps->doPlaySound();
        //gpio_set_level(GPIO_NUM_15, 0); // LED aus
    }

    /* Go back to sleep, only the ULP Risc-V will run */
    printf("Entering in deep sleep\n\n");

    /* Small delay to ensure the messages are printed */
    vTaskDelay(100);

    ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup());
    esp_deep_sleep_start();
}


// Load and start ULP Programm, wakeup period 20 ms
static void init_ulp_program(void)
{
    esp_err_t err = ulp_riscv_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
    ESP_ERROR_CHECK(err);

    /* The first argument is the period index, which is not used by the ULP-RISC-V timer
     * The second argument is the period in microseconds, which gives a wakeup time period of: 20ms
     */
    ulp_set_wakeup_period(0, 20000);

    /* Start the program */
    err = ulp_riscv_run();
    ESP_ERROR_CHECK(err);
}
