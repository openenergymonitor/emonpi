#!/usr/bin/python

import sys
import redis
import subprocess
import time
import signal

def handle_sigterm(sig, frame):
  print("Got Termination signal, exiting")
  sys.exit(0)

## Used to update log viewer window in Emoncms admin
# Used in conjunction with: service-runner-update.sh and Emoncms admin module

# Setup the signal handler to gracefully exit
signal.signal(signal.SIGTERM, handle_sigterm)
signal.signal(signal.SIGINT, handle_sigterm)

print("Starting service-runner")
sys.stdout.flush()

server = redis.Redis()
while True:
  try:
    if server.exists('service-runner'):
      flag = server.lpop('service-runner')
      print("Got flag: %s\n" % flag)
      sys.stdout.flush()
      script, logfile = flag.split('>')
      cmdstring = "{s} > {l}".format(s=script, l=logfile)
      print("STARTING: " + cmdstring)
      sys.stdout.flush()
      subprocess.call(cmdstring, shell=True)
      if not (os.path.isfile(logfile)):
        f = open(logfile, 'a')
        f.close()
      print("COMPLETE: " + cmdstring)
      sys.stdout.flush()
  except:
    print("Exception occurred", sys.exc_info()[0])
  time.sleep(0.2)

