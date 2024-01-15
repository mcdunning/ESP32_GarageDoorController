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
#include "driver/ledc.h"

#include "rgb_led.h"

ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM];
SemaphoreHandle_t counting_sem;

// handle for rgb_led_pwm_init
bool g_pwm_init_handle = false;

/**
 * This callack function will be called when the fade opertaion has ended
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
    counting_sem = xSemaphoreCreateCounting(LEDC_TEST_CH_NUM, 0);

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
    for(int ch = 0; ch < RGB_LED_CHANNEL_NUM; ch++)
    {
        ledc_channel_config(&ledc_channel[ch]);
        ledc_cb_register(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, &callbacks, (void *) counting_sem);
    }

    g_pwm_init_handle = true;
}

void rgb_led_set_color (const rgb_color_def_t *color)
{
    if (g_pwm_init_handle == false) 
    {
        rgb_led_pwm_init();
    }

    // Value shoud be 0 - 255 for 8 bit number
    ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, color->red);
    ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);

    ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, color->green);
    ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);

    ledc_set_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel, color->blue);
    ledc_update_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel);
}

void rgb_led_set_blink(const bool enableBlink) {
    if (g_pwm_init_handle == false) 
    {
        rgb_led_pwm_init();
    }

    if (enableBlink == true)
    {
        // TODO:  This will only fire once need to put into a running task
        // fade down to 0
        for (int ch = 0; ch < LEDC_TEST_CH_NUM; ch++)
        {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }

        for (int i = 0; i < LEDC_TEST_CH_NUM; i++) {
            xSemaphoreTake(counting_sem, portMAX_DELAY);
        }

        // fade up to max
        for (int ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }
    }
    else {
        for (int ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_TEST_DUTY);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
    }
    
} 