/**
 * rgb_led.h
 * 
 * Created on:  5 Jan 2024
 *     Author:  Matt Dunning
 */

#ifndef MAIN_RGB_LED_H_
#define MAIN_RGB_LED_H_

// TODO:  Define addiontal LED GPIO Pins here
// Move these values out of this class to make it more generic for the entire application?
// RGB LED GPIOs
#define RGB_LED_RED_GPIO        21
#define RGB_LED_GREEN_GPIO      22
#define RGB_LED_BLUE_GPIO       23

// RGB LED color mix channels
#define RGB_LED_CHANNEL_NUM     3

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

#endif // MAIN_RGB_LED_H_