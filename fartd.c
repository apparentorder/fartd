/*-
 * Copyright (c) 2016, Dennis Koegel
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * for reference,
 * see https://www.raspberrypi.org/learning/fart-detector/worksheet/
 * and README.md
 */

#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <libgpio.h>

const int sleep_calibrate = 0;
const int sleep_main = 500000;

#define LEVEL_GREEN   6
#define LEVEL_YELLOW  10
#define LEVEL_RED     14
#define LEVEL_UNKNOWN 16

const int resistorpin[4] = /* R1, R2, R3, R4 */ {17, 18, 22, 23};
const int detectpin = 4;
const int ledred = 21;
const int ledyellow = 20;
const int ledgreen = 16;

gpio_handle_t gpio;

char *datestr;
time_t now;

int is_daemon = 1;

void dacset (int dacval) {
	int i;

	for (i = 0; i < 4; i++) {
		if (dacval & (1 << i)) {
			/* mode OUTPUT: enabled, "siphon" current */
			gpio_pin_output(gpio, resistorpin[i]);
			gpio_pin_low(gpio, resistorpin[i]);
		} else {
			/* mode INPUT: disabled */
			gpio_pin_input(gpio, resistorpin[i]);
		}
	}
}

int calibrate() {
	int i;
	gpio_value_t val;

	/*
	 * 'i' can go to 16, because it could be the case that
	 * even with all resistors activated, it's STILL not
	 * enough to bring the sensor voltage below the detection
	 * threshold (to bring detectpin to LOW)
	 */

	for (i = 0; i<16; i++) {
		if (!is_daemon)
			printf(" %2d", i);

		dacset(i);

		if (sleep_calibrate)
			usleep(sleep_calibrate);

		val = gpio_pin_get(gpio, detectpin);
		if (val == GPIO_PIN_LOW) {
			if (!is_daemon)
				printf(".\n");

			return i;
		}
	}

	if (!is_daemon)
		printf(" OMGWTF.\n");

	return 16;
}

void setled(int air) {
	gpio_pin_set(gpio, ledgreen,   (air >= LEVEL_GREEN  ? GPIO_PIN_HIGH : GPIO_PIN_LOW));
	gpio_pin_set(gpio, ledyellow,  (air >= LEVEL_YELLOW ? GPIO_PIN_HIGH : GPIO_PIN_LOW));

	/* red: special case: blinking for UNKNOWN is handled in main */
	if (air >= LEVEL_RED)
		gpio_pin_high(gpio, ledred);
	else if (air < LEVEL_RED)
		gpio_pin_low(gpio, ledred);
}

char *airtostr(int air) {
	char *ret;

	if (air == LEVEL_UNKNOWN)
		asprintf(&ret, "<beyond measurement>");
	else if (air >= LEVEL_RED)
		asprintf(&ret, "%d (RED)", air);
	else if (air >= LEVEL_YELLOW)
		asprintf(&ret, "%d (yellow)", air);
	else if (air >= LEVEL_GREEN)
		asprintf(&ret, "%d (green)", air);
	else
		asprintf(&ret, "%d (-)", air);

	return ret;
}

int main (int argc, char **argv) {
	int air;
	char *airstr;
	int prevair = 0;
	char *prevairstr;

	if (argc == 2 && strcmp(argv[1], "-d") == 0)
		is_daemon = 0;

	gpio = gpio_open(0);

	if (gpio == GPIO_INVALID_HANDLE)
		err(69, "gpio_open() failed");

	if (!is_daemon)
		setbuf(stdout, NULL);
	else {
		switch (fork()) {
			case -1:	err(69, "fork()");
			case 0:		break;
			default:	exit(0);
		}

		openlog("fartd", LOG_NDELAY, LOG_DAEMON);

		/*
		 * if daemon, turn on all LEDs on at startup
		 * for 2 * main cycle time, to say hello
		 */
		setled(15);
		usleep(sleep_main * 2);
	}

	gpio_pin_input(gpio, detectpin);
	gpio_pin_output(gpio, ledgreen);
	gpio_pin_output(gpio, ledyellow);
	gpio_pin_output(gpio, ledred);

	if (!is_daemon)
		printf("initial value: ");

	prevair = air = calibrate();

	if (is_daemon) {
		airstr = airtostr(air);
		syslog(LOG_NOTICE, "Starting -- initial air quality: %s", airstr);
		free(airstr);
	}

	setled(air);

	for (;;) {
		usleep(sleep_main);

		if (!is_daemon) {
			time(&now);
			datestr = ctime(&now);
			datestr[strlen(datestr) - 1] = '\0';
			printf("%s: ", datestr);
		}

		air = calibrate();

		/* red alert: special case: blink for maximum value */
		if (air == LEVEL_UNKNOWN)
			gpio_pin_toggle(gpio, ledred);

		if (prevair == air)
			/* value unchanged - do nothing */
			continue;

		airstr = airtostr(air);
		prevairstr = airtostr(prevair);

		syslog(LOG_NOTICE, "Air quality changed: %s -> %s",
			prevairstr, airstr);

		free(airstr);
		free(prevairstr);

		setled(air);
		prevair = air;
	}

	return 0;
}

