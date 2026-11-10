#!/usr/bin/env python3
"""Stream a serial port to stdout, flushing every chunk (no fixed duration)."""
import os, sys, select, termios, tty, time

port = sys.argv[1]
fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
tty.setraw(fd)
attrs = termios.tcgetattr(fd)
attrs[4] = attrs[5] = termios.B115200
termios.tcsetattr(fd, termios.TCSANOW, attrs)
while True:
    r, _, _ = select.select([fd], [], [], 0.5)
    if not r:
        continue
    try:
        chunk = os.read(fd, 4096)
    except BlockingIOError:
        continue
    if not chunk:
        break
    sys.stdout.buffer.write(chunk)
    sys.stdout.buffer.flush()
