/**
 * rgb_led.c
 * 
 * Created on:  5 Jan 2024
 *     Author:  Matt Dunning
 */

#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/ledc.h"

#include "rgb_led.h"

static const char* TAG = "rgb_led";

ledc_channel_config_t ledc_channel[LEDC_CH_NUM];
SemaphoreHandle_t counting_sem;

static TaskHandle_t fade_handle;            // handle for the fade task
static int current_color[LEDC_CH_NUM];      // refernce to the current selected colodr for the fade
static  bool g_pwm_init_handle = false;     // handle for rgb_led_pwm_init

/**
 * This callack function will be called when the fade opertaion has ended
 * 
 * Use callback only if you are are aware it is being called inside an ISR
 * Otherwise, use a semaphore to unblock tasks
 **/
static IRAM_ATTR bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg)
{
    portBASE_TYPE taskAwoken = pdFALSE;

    if(param->event == LEDC_FADE_END_EVT)
    {
        SemaphoreHandle_t counting_sem = (SemaphoreHandle_t) user_arg;
        xSemaphoreGiveFromISR(counting_sem, &taskAwoken);
    }

    return (taskAwoken == pdTRUE);
}

static void rgb_led_fade_task(void * pvParameters) 
{
    while (1)
    {
         // fade down to 0
        for (int ch = 0; ch < LEDC_CH_NUM; ch++)
        {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }

        // for (int i = 0; i < LEDC_CH_NUM; i++) {
        //     xSemaphoreTake(counting_sem, portMAX_DELAY);
        // }

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // fade up to current color
        for (int ch = 0; ch < LEDC_CH_NUM; ch++) {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, current_color[ch], LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // for (int i = 0; i < LEDC_CH_NUM; i++) {
        //     xSemaphoreTake(counting_sem, portMAX_DELAY);
        // }
    }
}

/**
 * Initialize the RGB LED settings per channel, include
 * the GPI for each color, mode and timer configuration. 
 */
static void rgb_led_pwm_init(void)
{

    // Initialize fade service
    ledc_fade_func_install(0);
    ledc_cbs_t callbacks = {
        .fade_cb = cb_ledc_fade_end_event
    };
    counting_sem = xSemaphoreCreateCounting(LEDC_CH_NUM, 0);

    // Configure the timer
    ledc_timer_config_t ledc_timer = 
    {
        #if CONFIG_IDF_TARGET_ESP32
            .speed_mode         = LEDC_HS_MODE,         // timer mode
            .timer_num          = LEDC_HS_TIMER,        // timer index
        #else
            .speed_mode         = LEDC_LS_MODE,         // timer mode
            .timer_num          = LEDC_LS_TIMER,        // timer index
        #endif
        .duty_resolution    = LEDC_TIMER_13_BIT,    // resolution of PWM duty
        .freq_hz            = 5000,                 // frequency of PWM signal
        .clk_cfg            = LEDC_AUTO_CLK,        // Auto select the source clock
        
    };
    // Set configuration of the timer
    ledc_timer_config(&ledc_timer);

    // Configure the Channels
    
    #if CONFIG_IDF_TARGET_ESP32
        ledc_channel[0].channel    = LEDC_HS_CH0_CHANNEL;
        ledc_channel[0].duty       = 0;
        ledc_channel[0].gpio_num   = LEDC_HS_CH0_GPIO;
        ledc_channel[0].speed_mode = LEDC_HS_MODE;
        ledc_channel[0].hpoint     = 0;
        ledc_channel[0].timer_sel  = LEDC_HS_TIMER;
        ledc_channel[0].flags.output_invert = 0;
        
        ledc_channel[1].channel    = LEDC_HS_CH1_CHANNEL;
        ledc_channel[1].duty       = 0;
        ledc_channel[1].gpio_num   = LEDC_HS_CH1_GPIO;
        ledc_channel[1].speed_mode = LEDC_HS_MODE;
        ledc_channel[1].hpoint     = 0;
        ledc_channel[1].timer_sel  = LEDC_HS_TIMER;
        ledc_channel[1].flags.output_invert = 0;

        ledc_channel[2].channel    = LEDC_HS_CH2_CHANNEL;
        ledc_channel[2].duty       = 0;
        ledc_channel[2].gpio_num   = LEDC_HS_CH2_GPIO;
        ledc_channel[2].speed_mode = LEDC_HS_MODE;
        ledc_channel[2].hpoint     = 0;
        ledc_channel[2].timer_sel  = LEDC_HS_TIMER;
        ledc_channel[2].flags.output_invert = 0;
    #else
        ledc_channel[0].channel    = LEDC_LS_CH0_CHANNEL;
        ledc_channel[0].duty       = 0;
        ledc_channel[0].gpio_num   = LEDC_LS_CH0_GPIO;
        ledc_channel[0].speed_mode = LEDC_LS_MODE;
        ledc_channel[0].hpoint     = 0;
        ledc_channel[0].timer_sel  = LEDC_LS_TIMER;
        ledc_channel[0].flags.output_invert = 0;
    
        ledc_channel[1].channel    = LEDC_LS_CH1_CHANNEL;
        ledc_channel[1].duty       = 0;
        ledc_channel[1].gpio_num   = LEDC_LS_CH1_GPIO;
        ledc_channel[1].speed_mode = LEDC_LS_MODE;
        ledc_channel[1].hpoint     = 0;
        ledc_channel[1].timer_sel  = LEDC_LS_TIMER;
        ledc_channel[1].flags.output_invert = 0;

        // TODO add third channel for low speed mode
    #endif
       

    // Set LED Controller with previously prepaired configuration
    for(int ch = 0; ch < LEDC_CH_NUM; ch++)
    {
        ledc_channel_config(&ledc_channel[ch]);
        ledc_cb_register(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, &callbacks, (void *) counting_sem);
    }

    // Create the fade task and pause it
    // xTaskCreatePinnedToCore(rgb_led_fade_task, "rgb_led_fade_task", RGB_LED_FADE_TASK_STACK_SIZE, NULL, RGB_LED_FADE_TASK_PRIORITY, &fade_handle, RGB_LED_FADE_TASK_CORE_ID);
    // xTaskCreate(rgb_led_fade_task, "rgb_led_fade_task", RGB_LED_FADE_TASK_STACK_SIZE ,NULL, RGB_LED_FADE_TASK_PRIORITY, &fade_handle);
    // vTaskSuspend(fade_handle);

    g_pwm_init_handle = true;
}

/**
 * Sets the color of the RGB led.  If the fade task is running the led color values will be updated after during
 * the fade cycle.
 **/
void rgb_led_set_color (const rgb_color_def_t *color)
{
    if (!g_pwm_init_handle) 
    {
        rgb_led_pwm_init();
    }

    current_color[0] = color->red;
    current_color[1] = color->green;
    current_color[2] = color->blue;

    // Only set the color if the fade task is suspended.
    // The new color will be picked up when the led fades from 0 to current color
    // if (eSuspended == eTaskGetState(fade_handle)) {
        for (int ch = 0; ch < LEDC_CH_NUM; ch++) 
        {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, current_color[ch]);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
    // }
}

void rgb_led_start_blink(const bool startBlink) {
    if (!g_pwm_init_handle) 
    {
        rgb_led_pwm_init();
    }

    if (startBlink)
    {
        ESP_LOGI(TAG, "Fade Handle Task State = %d", eTaskGetState(fade_handle));
        if (eSuspended == eTaskGetState(fade_handle)) 
        {
            ESP_LOGI(TAG, "Starting Fade Task");
            vTaskResume(fade_handle);
        }
        else
        {
            ESP_LOGI(TAG, "Fade task is already running");
        }
    }
    else 
    {
        if (eRunning == eTaskGetState(fade_handle))
        {
            ESP_LOGI(TAG, "Pausing Fade Task");
            vTaskSuspend(fade_handle);
        }
        else
        {
            ESP_LOGI(TAG, "Fade task is already suspended");
        }
    }
    
} 