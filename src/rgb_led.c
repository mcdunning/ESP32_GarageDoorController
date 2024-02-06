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
#include "string.h"

#include "rgb_led.h"

static const char* TAG = "rgb_led";
static const int fast_blink_time = 1000;
static const int slow_blink_time = 3000;

ledc_channel_config_t ledc_channel[LEDC_CH_NUM];
SemaphoreHandle_t counting_sem;

static TaskHandle_t slow_fade_handle;       // handle for the slow fade task
static TaskHandle_t fast_fade_handle;       // handle for the fast fade task
static int current_color[LEDC_CH_NUM];      // reference to the current selected color for the fade
static eBlinkState current_blink_state;     // reference to the current blink state of the LED
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
    int *pvFadeTime = pvParameters;
    
    while (1)
    {
         // fade down to 0
        for (int ch = 0; ch < LEDC_CH_NUM; ch++)
        {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0, *pvFadeTime);
            ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }

        for (int i = 0; i < LEDC_CH_NUM; i++) {
            xSemaphoreTake(counting_sem, portMAX_DELAY);
        }

        // fade up to current color
        for (int ch = 0; ch < LEDC_CH_NUM; ch++) {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, current_color[ch], *pvFadeTime);
            ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }

        for (int i = 0; i < LEDC_CH_NUM; i++) {
            xSemaphoreTake(counting_sem, portMAX_DELAY);
        }
    }
}

/**
 * Initialize the RGB LED settings per channel, include
 * the GPI for each color, mode and timer configuration. 
 */
static void rgb_led_pwm_init(void)
{
    // Initialize the starting color
    memcpy(current_color, ((int[]){0, 0, 0}), sizeof(current_color));
    
    // Initialize the blink state
    current_blink_state = eConstant;
    
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

    // Create the slow fade task and pause it
    xTaskCreate(rgb_led_fade_task, 
                "rgb_led_slow_fade_task", 
                RGB_LED_FADE_TASK_STACK_SIZE,
                (void *) &slow_blink_time, 
                RGB_LED_FADE_TASK_PRIORITY, 
                &slow_fade_handle);
    vTaskSuspend(slow_fade_handle);
    
    // Create the fast fade task and pause it
    xTaskCreate(rgb_led_fade_task, 
                "rgb_led_fast_fade_task", 
                RGB_LED_FADE_TASK_STACK_SIZE,
                (void *) &fast_blink_time, 
                RGB_LED_FADE_TASK_PRIORITY, 
                &fast_fade_handle);
    vTaskSuspend(fast_fade_handle);

    g_pwm_init_handle = true;
}

/**
 * Sets the color of the RGB led.  If the fade task is running the led color values will be updated after during
 * the fade cycle.
 **/
void rgb_led_set_color (const int color[])
{
    if (!g_pwm_init_handle) 
    {
        rgb_led_pwm_init();
    }

     memcpy(current_color, color, sizeof(current_color));

    // Only set the color if the fade tasks are suspended.
    // The new color will be picked up when the led fades from 0 to current color
    if (eSuspended == eTaskGetState(slow_fade_handle) || eSuspended == eTaskGetState(fast_fade_handle)) {
        for (int ch = 0; ch < LEDC_CH_NUM; ch++) 
        {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, current_color[ch]);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
    }
}

void rgb_led_handle_blink(const eBlinkState blinkState) {
    if (!g_pwm_init_handle) 
    {
        rgb_led_pwm_init();
    }

    if (current_blink_state != blinkState) 
    {
        switch (blinkState)
        {
            case eConstant:
                if (eFastBlink == current_blink_state)
                {
                    ESP_LOGI(TAG, "Stopping the fast fade task");
                    vTaskSuspend(fast_fade_handle);
                } else {
                    ESP_LOGI(TAG, "Stopping the slow fade task");
                    vTaskSuspend(slow_fade_handle);
                }
                break;
            case eSlowBlink:
                if (eFastBlink == current_blink_state)
                {
                    ESP_LOGI(TAG, "Stopping the fast fade task");
                    vTaskSuspend(fast_fade_handle);
                }

                ESP_LOGI(TAG, "Starting the slow fade task");
                vTaskResume(slow_fade_handle);
                
                current_blink_state = blinkState;
                break;
            case eFastBlink:
               if (eSlowBlink == current_blink_state)
                {
                    ESP_LOGI(TAG, "Stopping the slow fade task");
                    vTaskSuspend(slow_fade_handle);
                }

                ESP_LOGI(TAG, "Starting the fast fade task");
                vTaskResume(fast_fade_handle);
                
                current_blink_state = blinkState;
                break;
            default:
                ESP_LOGW(TAG, "An invalid blink state was passed: %d", blinkState);
                break;
        }
    }   
} 