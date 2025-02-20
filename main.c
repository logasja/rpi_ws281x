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

#define TIMEOUT_SECS			30
#define TIMEOUT_USECS			0

#define LED_COUNT               2

int port = PORT;
int sock;

int clear_on_exit = 1;

static volatile uint8_t running = 1;
static uint8_t initialized = 0;
static uint8_t connected = 0;

void parseargs(int argc, char **argv)
{
	int index;
	int c;

	static struct option longopts[] =
	{
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{"port", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};

	while (1)
	{

		index = 0;
		c = getopt_long(argc, argv, "hvp:", longopts, &index);

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

static void ctrl_c_handler(int sig)
{
	signal(sig, SIG_IGN);
	running = 0;
}

static void setup_handlers(void)
{
	signal(SIGINT, ctrl_c_handler);
	signal(SIGSEGV, ctrl_c_handler);
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

	// Socket options
	struct timeval tv;
	tv.tv_sec = TIMEOUT_SECS;
	tv.tv_usec = TIMEOUT_USECS;
	setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	// Bind
	if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		puts("bind failed");
		exit(-1);
	}
	puts("bind done");

	return socket_desc;
}

#define MAXARGS		64

void parsecommand(int argc, char **argv)
{
	int opt;

	char* command = argv[0];

	fprintf(stderr,"Command: %s\n", argv[0]);

	if(command == NULL){
		connected=0;
	}
	else if(!strcmp(command, "setup"))
	{
		fprintf(stderr, "In setup\n");
		while ((opt = getopt(argc, argv, "n:f:g:d:t:b:i")) != -1)
		{
			fprintf(stderr, "%s", optarg);
			switch(opt) {
				case 'n': set_ledstring_n_led(atoi(optarg)); break;
				case 'f': set_ledstring_target_freq(atoi(optarg)); break;
				case 'g': set_ledstring_gpio_pin(atoi(optarg)); break;
				case 'd': set_ledstring_dma(atoi(optarg)); break;
				case 't': set_ledstring_strip_type(atoi(optarg)); break;
				case 'b': set_ledstring_global_brightness(atoi(optarg)); break;
				case 'i': set_ledstring_invert(1); break;
				default: break;
			}
		}
	} 
	else if (!strcmp(command, "init\n")) 
	{
		fprintf(stderr, "In init\n");
		// Stop render loop
		initialized = 0;
		reinit_ledstring();
		// Restart render loop
		initialized = 1;
	} 
	else if (!strcmp(command, "write"))
	{
		fprintf(stderr, "In write\n");
		int idx = 0;
		ws2811_led_t color = 0;
		while ((opt = getopt(argc, argv, "i:c:")) != -1)
		{
			fprintf(stderr, "%s", optarg);
			switch(opt) {
				case 'i': idx = atoi(optarg); break;
				case 'c': color = strtol(optarg, NULL, 16); break;
				default: break;
			}
		}
		fprintf(stderr, "Setting %d to %x\n", idx, color);
		write_led(idx, color);
	}
	else if (!strcmp(command, "fill"))
	{
		fprintf(stderr, "In fill\n");
		ws2811_led_t color = 0;
		while((opt = getopt(argc, argv, "c:")) != -1)
		{
			switch(opt) {
				case 'c': color = strtol(optarg, NULL, 16); break;
				default: break;
			}
		}
		fill_leds(color);
	}
	else {
		fprintf(stderr, "Not a command\n");
	}

	optind = 1;
}

void execute_command(char* command)
{
	int argc = 0;
	char *argv[MAXARGS];

	char *p1 = strtok(command, " ");
	while (p1 && argc < MAXARGS-1)
	{
		argv[argc++] = p1;
		p1 = strtok(0, " ");
	}
	argv[argc] = 0;
	parsecommand(argc, argv);
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

	while(running) {
		listen(sock, 3);
		client_socket = accept(sock, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_socket < 0) {
			fprintf(stderr, "Timeout\n");
			continue;
		}
		
		connected=1;

		int bytes = 0;

		while(connected) {
			for(int i=0; i < MAXMSG; i++)
			{
				recieve[i] = 0;
			}
			bytes = read(client_socket, recieve, MAXMSG);
			if (bytes < 0)
			{
				fprintf(stderr,"Timeout");
				break;
			}
			fprintf(stderr, recieve);
			execute_command(recieve);
		}
		close(client_socket);
		connected=0;
	}

	close(sock);
	fprintf(stderr, "Exit Socket Listener\n");
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

	fprintf(stderr, "Exit Render\n");
	pthread_exit(NULL);
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

	while(running && !connected){
		clear_ledstring();
		usleep(1000000);
		write_led(0, 0x00050505);
		usleep(1000000);
	}

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
