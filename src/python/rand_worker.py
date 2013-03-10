import socket
import sys
import subprocess
import time
import random

print len(sys.argv)
if len(sys.argv) != 4:
  print 'not enough input args'
  sys.exit(-1)

DEBUG = True;
TMPFS_PATH = '/dev/shm/';
WORKER_PORT = 11110

size = int(sys.argv[3])

myhost = socket.gethostname()
cnn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
cnn.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
cnn.connect((myhost, WORKER_PORT))
cnn.send('alive')

while True:

  mesg = cnn.recv(8192)

  if mesg == 'kill':
    print 'Time to die!'
    break

  params = mesg.split(',')
  print params
  jobid = params[2].strip()
  token = int(params[4].strip())
  dest_task = random.randint(1,size-1)

  token += 1

  cnn.send(jobid + ':' + str(dest_task) + ':' + str(token))
  
cnn.close()
