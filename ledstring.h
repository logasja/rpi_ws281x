/*
 * ledstring.h
 *
 * Copyright (c) 2016 Jacob Logas <logasja @ gmail.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     1.  Redistributions of source code must retain the above copyright notice, this list of
 *         conditions and the following disclaimer.
 *     2.  Redistributions in binary form must reproduce the above copyright notice, this list
 *         of conditions and the following disclaimer in the documentation and/or other materials
 *         provided with the distribution.
 *     3.  Neither the name of the owner nor the names of its contributors may be used to endorse
 *         or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __LEDSTRING_H__
#define __LEDSTRING_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ws2811.h"

// Initialization
int init_ledstring(int n_led, int target_freq, int gpio_pin, int dma, int strip_type);
int reinit_ledstring(void);

// Deinitialization
void fini_ledstring(void);

// Change ledstring struct
void set_ledstring_n_led(int n_led);
void set_ledstring_target_freq(int target_freq);
void set_ledstring_gpio_pin(int gpio_pin);
void set_ledstring_dma(int dma);
void set_ledstring_strip_type(int strip_type);
void set_ledstring_global_brightness(int global_brightness);
void set_ledstring_invert(int invert);

// Set LEDs to color
void write_led(int idx, ws2811_led_t val);
void write_leds(ws2811_led_t *vec);
void clear_ledstring(void);
void fill_leds(ws2811_led_t val);

// Calls render function, sends LED values to hardware
int render_ledstring(void);

// Retrieves current LED values
const ws2811_led_t* get_led_values(void);
ws2811_led_t get_led_value(int idx);

#ifdef __cplusplus
}
#endif

#endif /* __LEDSTRING_H__ */