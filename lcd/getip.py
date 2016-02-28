#!/usr/bin/env python
import subprocess
import sys
import signal


cmd = ' '
ip = ' '
# Dfault eth0
def getip( interface="eth0" ):
   global ip
   # returns 'down' if interface is down and '' if interface is up
   ifdown = subprocess.check_output("ip a | grep -Eq ': "+interface+":.*state' || echo down", shell=True)
   # If interface is NOT down then it must be up up >  get it's IP address
   if ifdown =='':
      cmd = "ip addr show "+interface+" | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
      ip = subprocess.check_output(cmd, shell=True).rstrip()
   else:
      ip = False
   if ip=="" or ip==False or (ip[:1].isdigit()!=1):
       return(False)
   else: 
      return(ip)

#print getip("eth0")
