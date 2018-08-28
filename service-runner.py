#!/usr/bin/python

## Used to update log viewer window in Emoncms admin
# Used in conjunction with: service-runner-update.sh and Emoncms admin module

import sys
import redis
import subprocess
import time
import signal
import os

def handle_sigterm(sig, frame):
  print("Got Termination signal, exiting")
  sys.exit(0)

# Setup the signal handler to gracefully exit
signal.signal(signal.SIGTERM, handle_sigterm)
signal.signal(signal.SIGINT, handle_sigterm)

def connect_redis():
  while True:
    try:
      server = redis.Redis()
      if server.ping():
        print("Connected to redis-server")
        sys.stdout.flush()
        return server
    except redis.exceptions.ConnectionError:
      print("Unable to connect to redis-server, sleeping for 30s")
      sys.stdout.flush()
    time.sleep(30)

print("Starting service-runner")
sys.stdout.flush()

server = connect_redis()

while True:
  try:
    # Check for the existence of a redis 'service-runner' key
    if server.exists('service-runner'):
      # We've got one, now to turn it into a cmdline
      flag = server.lpop('service-runner')
      print("Got flag: %s\n" % flag)
      sys.stdout.flush()
      script, logfile = flag.split('>')
      cmdstring = "{s} > {l} 2>&1".format(s=script, l=logfile)
      print("STARTING: " + cmdstring)
      sys.stdout.flush()
      # Got a cmdline, now run it.
      subprocess.call(cmdstring, shell=True)
      if not (os.path.isfile(logfile)):
        f = open(logfile, 'a')
        f.close()
      print("COMPLETE: " + cmdstring)
      sys.stdout.flush()
  except redis.exceptions.ConnectionError:
    print("Connection to redis-server lost, attempting to reconnect")
    sys.stdout.flush()
    server = connect_redis()
  except SystemExit:
    # If the sys.exit(0) from the interrupt handler gets caught here,
    # just break from the while True: and let the script exit normally.
    break
  except:
    print("Exception occurred", sys.exc_info()[0])
    sys.exit(1)
  time.sleep(0.2)

