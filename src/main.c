#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "sdkconfig.h"
#include "main.h"

#define HIGH 1
#define LOW 0
#define OUTPUT GPIO_MODE_OUTPUT
#define INPUT GPIO_MODE_INPUT

#define PIN_RED     23 // GPIO23
#define PIN_GREEN   22 // GPIO22
#define PIN_BLUE    21 // GPIO21

void setup() {
    gpio_set_direction(PIN_RED,    OUTPUT);
    gpio_set_direction(PIN_GREEN,  OUTPUT);
    gpio_set_direction(PIN_BLUE,   OUTPUT);

}

void setColor(int R, int G, int B) {
    gpio_set_level(PIN_RED,   R);
    gpio_set_level(PIN_GREEN, G);
    gpio_set_level(PIN_BLUE,  B);
}

void app_main(void) {
    printf("Hello world!\n\n");

    setColor(0, 201, 204);

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;

    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi " : "",
            (chip_info.features & CHIP_FEATURE_BT) ? " BT " : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? " BLE " : "",
            (chip_info.features & CHIP_FEATURE_IEEE802154) ? " 802.15.4 (Zigbee/Thread)" : ""
        );
    
    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("Getting Chip Information");
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    setColor(247, 120, 138);
    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    setColor(52, 168, 83);
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();

}