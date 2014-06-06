
from subprocess import *
from time import sleep, strftime
from datetime import datetime



 

 
# Return local IP address for eth0 or wlan0
def local_IP():
	wlan0 = "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
	eth0 = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
	p = Popen(eth0, shell=True, stdout=PIPE)
	IP = p.communicate()[0]
	network = "eth0"
	if IP == "":
		p = Popen(wlan0, shell=True, stdout=PIPE)
		IP = p.communicate()[0]
		network = "wlan0"
	return {IP , network}
IP, network = local_IP()
print IP
print network

#Test to see if we have working internet connection 
import socket
REMOTE_SERVER = "www.google.com"
def is_connected():
  try:
    # see if we can resolve the host name -- tells us if there is
    # a DNS listening
    host = socket.gethostbyname(REMOTE_SERVER)
    # connect to the host -- tells us if the host is actually
    # reachable
    s = socket.create_connection((host, 80), 2)
    return True
  except:
     pass
  return False
print is_connected()
       
