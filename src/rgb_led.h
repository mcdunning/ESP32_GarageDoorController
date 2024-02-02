/**
 * rgb_led.h
 * 
 * Created on:  5 Jan 2024
 *     Author:  Matt Dunning
 */

#ifndef MAIN_RGB_LED_H_
#define MAIN_RGB_LED_H_

// LEDC Outpt Config
// High Speed Channel Group
#if CONFIG_IDF_TARGET_ESP32
#define LEDC_HS_TIMER       LEDC_TIMER_0
#define LEDC_HS_MODE        LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO    (21)
#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH1_GPIO    (22)
#define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1
#define LEDC_HS_CH2_GPIO    (23)
#define LEDC_HS_CH2_CHANNEL LEDC_CHANNEL_2
#endif

//Low Speed Channel Group
#if !CONFIG_IDF_TARGET_ESP32
#define LEDC_LS_TIMER       LEDC_TIMER_1
#define LEDC_LS_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_HS_CH0_GPIO    (8)
#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH1_GPIO    (9)
#define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1
#define LEDC_LS_CH2_GPIO    (4)
#define LEDC_LS_CH2_CHANNEL LEDC_CHANNEL_2
#endif

#define LEDC_CH_NUM         (3)
#define LEDC_TEST_FADE_TIME (3000)

#define RGB_LED_FADE_TASK_STACK_SIZE 4096
#define RGB_LED_FADE_TASK_PRIORITY   5
#define RGB_LED_FADE_TASK_CORE_ID    1

// RGB LED configuration
typedef struct
{
    int channel;
    int gpio;
    int mode;
    int timer_index;
} ledc_info_t;

typedef struct 
{
    int red;
    int green;
    int blue;
} rgb_color_def_t;


/**
 * Sets the rgb led color based on the passed in data
 */
void rgb_led_set_color(const rgb_color_def_t *color);
void rgb_led_start_blink(const bool start_blink);

#endif // MAIN_RGB_LED_H_