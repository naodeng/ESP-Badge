#!/usr/bin/env python3
"""Read a serial port for a fixed number of seconds (stdlib only).

Usage:  python3 agent-badge/tools/serial_read.py /dev/cu.usbmodem101 [seconds]
Prints decoded UTF-8 text (replace errors) to stdout.
"""
import sys
import os
import select
import termios
import time
import tty


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    port = sys.argv[1]
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 8.0

    fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        attrs = termios.tcgetattr(fd)
        tty.setraw(fd)
        attrs = termios.tcgetattr(fd)
        attrs[4] = attrs[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, attrs)

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
