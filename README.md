fartd
=====

Implementation of the Fart Detector suggested by the
Raspberry Pi Foundation here:

    https://www.raspberrypi.org/learning/fart-detector/

This is an alternative software implementation for the given hardware setup:

 * It's written in C
 * It uses the gpio(3) library provided since FreeBSD 11
 * It is a daemon that logs air quality changes as they happen, using `syslog(3)`
 * It constantly shows status with three LEDs (intended as green/yellow/red)
 * 2c BSD License

Installation
------------

Simply run `make all install`.
It will compile and install the awesome `fartd` binary, along with an appropriate RCng script.
All you need to do is to set `fartd_enable=yes` in your `rc.conf`.

Requirements
------------

**Be sure to double-check all the Pin numbers that are hardcoded in the `.c` file!**
You can change them as needed, of course.

The `resistorpins[4]` correspond to resistors R1, R2, R3, R4 and are configured
to the same pins as they are in the reference implementation (GPIO 17, 18, 22, 23).
In the same manner, the detection pin is expected at GPIO 4.

The LEDs are expected at:
 * Green: GPIO 16
 * Yellow: GPIO 20
 * RED: GPIO 21

Tuning
------

If necessary, you can adjust the `LEVEL_*` constants to have different thresholds.

If air quality is actually *better* than a certain level, no LED is active at all.
The expected normal condition is something in between (only the green LED is active).

Debugging
---------

If you run `fartd -d`, it won't `fork(2)` and will print to STDOUT instead of syslog.

Also you may want to increase `sleep_calibrate` (default disabled) to check
the activation of the individual resistor ladder combinations.

Feedback
--------

You're welcome to provide feedback to <dk@neveragain.de> (or via github).
Especially if you happen to connect this to an SNMP daemon somehow, please let me know.

