/*
 * newtest.c
 *
 * Copyright (c) 2014 Jeremy Garff <jer @ jers.net>
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


static char VERSION[] = "XX.YY.ZZ";

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
#include <pthread.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "clk.h"
#include "gpio.h"
#include "dma.h"
#include "pwm.h"
#include "version.h"

#include "ws2811.h"
#include "ledstring.h"

#define PORT 9999
#define MAXMSG 100

#define ARRAY_SIZE(stuff)       (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                18
#define DMA                     10
//#define STRIP_TYPE            WS2811_STRIP_RGB		// WS2812/SK6812RGB integrated chip+leds
#define STRIP_TYPE              WS2811_STRIP_GBR		// WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW		// SK6812RGBW (NOT SK6812RGB)

#define LED_COUNT               2

int port = PORT;
int sock;

int clear_on_exit = 0;

static uint8_t running = 1;
static uint8_t initialized = 0;
static uint8_t connected = 0;

void parseargs(int argc, char **argv)
{
	int index;
	int c;

	static struct option longopts[] =
	{
		{"help", no_argument, 0, 'h'},
		{"dma", required_argument, 0, 'd'},
		{"gpio", required_argument, 0, 'g'},
		{"invert", no_argument, 0, 'i'},
		{"clear", no_argument, 0, 'c'},
		{"strip", required_argument, 0, 's'},
		{"led_count", required_argument, 0, 'x'},
		{"version", no_argument, 0, 'v'},
		{"port", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};

	while (1)
	{

		index = 0;
		c = getopt_long(argc, argv, "cd:g:his:vx:y:", longopts, &index);

		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			/* handle flag options (array's 3rd field non-0) */
			break;

		case 'h':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			fprintf(stderr, "Usage: %s \n"
				"-h (--help)    - this information\n"
				"-p (--port)  	- sets tcp port\n"
				"-v (--version) - version information\n"
				, argv[0]);
			exit(-1);

		case 'p':
			port = atoi(optarg);
			break;

		case 'v':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			exit(-1);

		case '?':
			/* getopt_long already reported error? */
			exit(-1);

		default:
			exit(-1);
		}
	}
}

static void ctrl_c_handler(int signum)
{
	(void)(signum);
    running = 0;
}

static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int socket_init(void)
{
	int socket_desc;
	struct sockaddr_in server;

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc == -1)
	{
		printf("Could not create socket");
	}

	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(port);

	// Bind
	if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		puts("bind failed");
	}
	puts("bind done");

	return socket_desc;
}

void *listener(void *threadid)
{
	long tid;
	tid = (long)threadid;
	fprintf(stderr, "Spawned %lo\n", tid);
	sock = socket_init();
	int client_socket, c;
	struct sockaddr_in client;

	char recieve[MAXMSG];

	listen(sock, 3);
	client_socket = accept(sock, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_socket < 0) {
		fprintf(stderr,"Accept Failed");
	}
	connected=1;

	int bytes = 0;

	while(running) {
		for(int i=0; i < MAXMSG; i++)
		{
			recieve[i] = 0;
		}
		bytes = read(client_socket, recieve, MAXMSG);
		if (bytes < 0)
		{
			fprintf(stderr,"Read Error!");
		}
		printf(recieve);
	}
	close(sock);
	close(client_socket);
	connected=0;
	pthread_exit(NULL);
}

void *render(void *threadid)
{
	long tid;
	tid = (long)threadid;
	fprintf(stderr, "Spawned %lo\n", tid);

	while(running) {
		if(initialized) {
			render_ledstring();
		}
		// 30 frames /sec
		usleep(1000000 / 30);
	}
	pthread_exit(NULL);
}

void startup() 
{
	int idx = 0;
	while(!connected)
	{
		++idx;
		idx %= LED_COUNT;
		if(get_led_value(idx) == 0x00050500)
		{
			write_led(idx, 0x00050505);
		} else if (get_led_value(idx) == 0x00050505) {
			write_led(idx, 0x00000505);	
		}else {
			write_led(idx, 0x00050500);
		}
		// 30 frames /sec
		usleep(1000000 / 15);
	}
	clear_ledstring();
}

void test()
{
	initialized = 0;
	clear_ledstring();
	render_ledstring();
	set_ledstring_n_led(12);
	reinit_ledstring();
	initialized = 1;

	int idx = 0;
	while(connected)
	{
		++idx;
		idx %= 12;
		if(get_led_value(idx) == 0x00505000)
		{
			write_led(idx, 0x00505050);
		} else if (get_led_value(idx) == 0x00505050) {
			write_led(idx, 0x000005050);	
		}else {
			write_led(idx, 0x00505000);
		}
		usleep(1000000 / 15);
	}
	clear_ledstring();
}

int main(int argc, char *argv[])
{
	int rc;

    sprintf(VERSION, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);

	parseargs(argc, argv);

    setup_handlers();

	if(init_ledstring(LED_COUNT, 
					  TARGET_FREQ, 
					  GPIO_PIN, 
					  DMA, 
					  STRIP_TYPE) != WS2811_SUCCESS)
	{
		exit(-1);
	}
	initialized = 1;

	// Spawn listener thread
	pthread_t sock_thread;
	printf("Spawning socket thread\n");
	rc = pthread_create(&sock_thread, NULL, listener, 0);
	if(rc) {
		fprintf(stderr, "ERROR: return code %d", rc);
		exit(-1);
	}

	// Spawn render thread
	pthread_t render_thread;
	printf("Spawning render thread\n");
	rc = pthread_create(&render_thread, NULL, render, 0);
	if(rc) {
		fprintf(stderr, "ERROR: return code %d", rc);
		exit(-1);
	}

	startup();

	test();

	pthread_join(sock_thread, NULL);
	pthread_join(render_thread, NULL);

    if (clear_on_exit) {
		clear_ledstring();
		render_ledstring();
    }

	fini_ledstring();

    fprintf (stderr,"\n");
	return 0;
}
