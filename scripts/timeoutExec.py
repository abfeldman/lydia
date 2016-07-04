# This code was writem by Amit Metody.


import subprocess
import signal
import time
import sys


SP = None

def handler(signum, frame):
	global SP
	if SP!=None and SP.poll()==None:
		SP.send_signal(signal.SIGTERM)
		time.sleep(1)
		print("------------- KILLED DUE TO TIMEOUT !!!! ----- ")
		if SP.poll()==None:
			SP.kill()
			SP.wait()

signal.signal(signal.SIGINT, handler)
signal.signal(signal.SIGTERM, handler)
signal.signal(signal.SIGALRM, handler)


print sys.argv[2:]
SP = subprocess.Popen(sys.argv[2:])
signal.alarm(int(sys.argv[1]))
try:
	SP.wait()
except:
	None
signal.alarm(0)
SP=None
