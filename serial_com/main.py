# -*- coding: utf-8 -*-

# region Description
"""
serial_com.py: Command line interface for ESP32
Author: johndeweyzxc (johndewey02003@gmail.com)
Usage: python main.py [PORT] [LOG TYPE]
Example Usage: python main.py COM4 info
"""
# endregion

from esp32_com import ESP32Com
import sys

OUT_YELLOW = "\033[93m"
OUT_GREEN = "\033[92m"
OUT_MAGENTA = "\033[35m"
OUT_CYAN = "\033[36m"
COLOR_RESET = "\033[0m"


def main(port: str, verbose: bool):
    ESP32Com(port, verbose).start_thread()


if __name__ == '__main__':
    port = sys.argv[1]
    verbose = sys.argv[2]
    if verbose == 'verbose':
        verbose = True
    elif verbose == 'info':
        verbose = False

    main(port, verbose)
