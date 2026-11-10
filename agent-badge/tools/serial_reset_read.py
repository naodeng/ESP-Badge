#!/usr/bin/env python3
"""Open a serial port, pulse the ESP32 reset line, then read for N seconds.

Usage:  python3 agent-badge/tools/serial_reset_read.py /dev/cu.usbmodem101 [seconds]

Uses the USB-Serial/JTAG control lines (RTS -> EN) so the boot log is captured
from the first line. Stdlib only.
"""
import sys
import os
import select
import termios
import fcntl
import time
import tty
import array

TIOCM_DTR = 0x002
TIOCM_RTS = 0x004


def set_lines(fd: int, dtr: bool, rts: bool) -> None:
    buf = array.array("i", [0])
    fcntl.ioctl(fd, termios.TIOCMGET, buf, True)
    bits = buf[0]
    bits = (bits | TIOCM_DTR) if dtr else (bits & ~TIOCM_DTR)
    bits = (bits | TIOCM_RTS) if rts else (bits & ~TIOCM_RTS)
    fcntl.ioctl(fd, termios.TIOCMSET, array.array("i", [bits]))


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    port = sys.argv[1]
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 8.0

    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        tty.setraw(fd)
        attrs = termios.tcgetattr(fd)
        attrs[4] = attrs[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, attrs)

        # ESP32-S3 USB-Serial/JTAG: RTS drives EN (reset), DTR drives GPIO0.
        set_lines(fd, dtr=False, rts=True)
        time.sleep(0.15)
        set_lines(fd, dtr=False, rts=False)
        time.sleep(0.05)

        deadline = time.time() + seconds
        buf = b""
        while time.time() < deadline:
            r, _, _ = select.select([fd], [], [], 0.2)
            if not r:
                continue
            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                continue
            if not chunk:
                break
            buf += chunk
        sys.stdout.write(buf.decode("utf-8", "replace"))
        sys.stdout.flush()
    finally:
        os.close(fd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
