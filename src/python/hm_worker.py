import socket
import sys
import subprocess
import time

print len(sys.argv)
if len(sys.argv) != 4:
  print 'not enough input args'
  sys.exit(-1)

DEBUG = True;
TMPFS_PATH = '/dev/shm/';
WORKER_PORT = 11110

print socket.gethostname()

myhost = socket.gethostname()

cnn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
cnn.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
cnn.connect((myhost, WORKER_PORT))
cnn.send('alive')

while True:
  mesg = cnn.recv(8192)
  print 'received data:', mesg
  if mesg == 'kill':
    print 'Time to die!'
    break
  params = mesg.split(',')
  infile = params[3].strip()
  jobid = params[2].strip()
  print "infile is " + infile
  out = subprocess.check_output('which HMtools', shell=True)
  print "out: " + out
  pout = subprocess.check_output('HMtools ' + infile + ' ' + myhost, shell=True) 
  print 'pout is:' + pout.strip()
  #pstat = subprocess.check_output(pout.strip(), stderr=open('myerr.err','w'), shell=True) 
  pstat = subprocess.check_output(pout.strip(), stderr=subprocess.STDOUT, shell=True) 

  log_file = pout.split('>')[-2].split()[0]

  print 'log_file is ' + log_file

  try:
    with open(log_file) as f: pass
  except IOError as e:
    print 'Oh dear.'
  
  cnn.send(jobid + ':' + log_file)
  
cnn.close()
