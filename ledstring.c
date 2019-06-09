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

ws2811_return_t init_ledstring(int n_led, int target_freq, int gpio_pin, int dma, int strip_type)
{
    ws2811_return_t ret;
    led_values = malloc(sizeof(ws2811_led_t) * n_led);

    led_struct.freq = target_freq;
    led_struct.dmanum = dma;
    
    led_struct.channel[0].gpionum = gpio_pin;
    led_struct.channel[0].count = n_led;
    led_struct.channel[0].invert = 0;
    led_struct.channel[0].brightness = 255;
    led_struct.channel[0].strip_type = strip_type;
    led_struct.channel[0].leds = led_values;

    led_struct.channel[1].gpionum = 0;
    led_struct.channel[1].count = 0;
    led_struct.channel[1].invert = 0;
    led_struct.channel[1].brightness = 0;

    if ((ret = ws2811_init(&led_struct)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
    }
    return ret;
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
