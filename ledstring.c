#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdarg.h>
#include <getopt.h>

#include "ledstring.h"

ws2811_t        led_struct;
ws2811_led_t    *led_values;

int g_gpionum, g_count, g_invert, g_brightness, g_strip_type, g_target_freq, g_dma;

ws2811_return_t init_struct(void)
{
    ws2811_return_t ret;

    led_struct.freq = g_target_freq;
    led_struct.dmanum = g_dma;
    
    led_struct.channel[0].gpionum = g_gpionum;
    led_struct.channel[0].count = g_count;
    led_struct.channel[0].invert = g_invert;
    led_struct.channel[0].brightness = g_brightness;
    led_struct.channel[0].strip_type = g_strip_type;
    led_struct.channel[0].leds = malloc(sizeof(ws2811_led_t) * g_count);

    led_struct.channel[1].gpionum = 0;
    led_struct.channel[1].count = 0;
    led_struct.channel[1].invert = 0;
    led_struct.channel[1].brightness = 0;

    led_values = led_struct.channel[0].leds;

    if ((ret = ws2811_init(&led_struct)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
    }
    return ret;
}

ws2811_return_t init_ledstring(int n_led, int target_freq, int gpio_pin, int dma, int strip_type)
{
    g_gpionum = gpio_pin;
    g_target_freq = target_freq;
    g_dma = dma;
    g_strip_type = strip_type;
    g_count = n_led;
    g_invert = 0;
    g_brightness = 255;

    return init_struct();
}

ws2811_return_t reinit_ledstring(void)
{
    // Deinitialize current ledstring
    ws2811_fini(&led_struct);

    return init_struct();
}

void fini_ledstring(void)
{
    ws2811_fini(&led_struct);
}

void write_led(int idx, ws2811_led_t val)
{
    if(idx >= 0 && idx < led_struct.channel[0].count){
        // printf("writing %d to %x\n", idx, val);
        led_values[idx] = val;
    }
}

void write_leds(ws2811_led_t *vec)
{
    for (int i = 0; i < led_struct.channel[0].count; i++)
    {
        led_values[i] = vec[i];
    }
}

void fill_leds(ws2811_led_t val)
{
    for (int i = 0; i < led_struct.channel[0].count; i++)
    {
        led_values[i] = val;
    }
}

void clear_ledstring(void)
{
    for(int i = 0; i < led_struct.channel[0].count; i++)
    {
        led_values[i] = 0;
    }
}


int render_ledstring(void)
{
    ws2811_return_t ret;
    led_struct.channel[0].leds = led_values;
    if ((ret = ws2811_render(&led_struct)) != WS2811_SUCCESS)
    {
        printf("ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
        return -1;
    }
    return 0;
}

const ws2811_led_t* get_led_values(void)
{
    return led_values;
}

ws2811_led_t get_led_value(int idx)
{
    if(idx >= 0 && idx < led_struct.channel[0].count) {
        return led_values[idx];
    } else {
        return -1;
    }
}

void set_ledstring_n_led(int n_led) {
    g_count = n_led;
}

void set_ledstring_target_freq(int target_freq) {
    g_target_freq = target_freq;
}

void set_ledstring_gpio_pin(int gpio_pin) {
    g_gpionum = gpio_pin;
}

void set_ledstring_dma(int dma) {
    g_dma = dma;
}

void set_ledstring_strip_type(int strip_type) {
    g_strip_type = strip_type % 12;
}

void set_ledstring_global_brightness(int global_brightness) {
    g_brightness = global_brightness % 256;
}

void set_ledstring_invert(int invert) {
    g_invert = invert % 2;
}
