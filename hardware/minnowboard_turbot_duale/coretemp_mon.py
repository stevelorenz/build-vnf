#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: CPU core temperature monitor for MinnowBoard Turbot E
       This is used to avoid overheating when the CPU fan is dissembled (Ruhe!)
       Run this script with root privilege
"""

import os
import time

import psutil

PEROID_S = 3
MIN_ALARM_TEMP = 60
# D2 LED uses GPIO 360
LED2 = 360
GPIO_EXPORT = "/sys/class/gpio/gpio/export"
GPIO_LED_ROOT = "/sys/class/gpio/gpio{}".format(LED2)
GPIO_LED_VAL = os.path.join(GPIO_LED_ROOT, "value")


def led_off():
    with open(GPIO_LED_VAL, "w") as f:
        f.write(str(1))


def blink_led(p=0.5):
    for _ in range(3):
        with open(GPIO_LED_VAL, "w") as f:
            f.write(str(0))
            time.sleep(p)
        with open(GPIO_LED_VAL, "w") as f:
            f.write(str(1))
            time.sleep(p)


def main():
    # Initialize GPIO
    if not os.path.exists(GPIO_LED_ROOT):
        with open(GPIO_EXPORT, "w") as f:
            f.write(LED2)

    with open(os.path.join(GPIO_LED_ROOT, "direction"), "w") as f:
        f.write("out")

    while True:
        cpu_temp = psutil.sensors_temperatures()["coretemp"][0].current
        if cpu_temp > MIN_ALARM_TEMP:
            blink_led()
        else:
            led_off()
        time.sleep(PEROID_S)


if __name__ == "__main__":
    main()
