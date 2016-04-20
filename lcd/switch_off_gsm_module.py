#!/usr/bin/env python
import time
import subprocess
import serial


ser = serial.Serial(port='/dev/ttyO4', baudrate=115200, bytesize=8,parity='N', stopbits=1, timeout=5)

subprocess.call(['/home/debian/gprsAndEmonInstall/gprs_off.sh'])
time.sleep(1)

cmd="AT+CPOWD=1\r"
ser.write(cmd.encode())
