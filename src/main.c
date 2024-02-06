#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "sdkconfig.h"
#include "rgb_led.h"

#define RGB_LED_FUNCTION_TEST_TASK_STACK_SIZE 4800
#define RGB_LED_FUNCTION_TEST_TASK_PRIORITY   6
#define RGB_LED_FUNCTION_TEST_TASK_CORE_ID    0

static const char* TAG = "main";

static const int color_cyan[3] = {0, 201, 204};
static const int color_salmon[3] = {247, 120, 138};
static const int color_melon[3] = {52, 168, 83};

// The testing task to drive program development
static void rgb_led_function_test_task(void * pvParmeters)
{
    for(;;)
    {
        ESP_LOGI(TAG, "Setting Color to Salmon");
        rgb_led_set_color(color_salmon);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Setting Color to Cyan");
        rgb_led_set_color(color_cyan);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    
        ESP_LOGI(TAG, "Setting color to Melon");
        rgb_led_set_color(color_melon);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Starting Slow Blink");
        rgb_led_handle_blink(eSlowBlink);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        ESP_LOGI(TAG, "Starting Fast Blink");
        rgb_led_handle_blink(eFastBlink);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Stoppint Blink");
        rgb_led_handle_blink(eConstant);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

    }
}

void app_main(void) {
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
    
    ESP_LOGI(TAG, "Starting RGB_LED function testing");
    xTaskCreatePinnedToCore(rgb_led_function_test_task, 
                            "rgb_led_function_test_task", 
                            RGB_LED_FUNCTION_TEST_TASK_STACK_SIZE, 
                            NULL, 
                            RGB_LED_FUNCTION_TEST_TASK_PRIORITY, 
                            NULL, 
                            RGB_LED_FUNCTION_TEST_TASK_CORE_ID);
    
}