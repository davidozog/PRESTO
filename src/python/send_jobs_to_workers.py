import subprocess
import socket
import sys

DEBUG = False;
TMPFS_PATH = '/dev/shm/';
MASTER_PORT = 11112

def get_uid():
  uid = subprocess.check_output('id')
  uid = uid.split()[0]
  lft = uid.index('=') + 1
  rht = uid.index('(')
  uid = uid[lft:rht]
  return uid

cnn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
cnn.connect(('localhost', MASTER_PORT))

# Here I'd like to construct a forward solution file and send it to 
# a worker.  For now, just feed two input files:

infiles = []
#infiles.append('./pair-i103s45.hm')
#infiles.append('./pair-i104s57.hm')
infiles.append('./cond_inputfile1.hm')
infiles.append('./cond_inputfile2.hm')
mesg = []
running_jobs = []

uid = get_uid()
fshared = open(TMPFS_PATH+'.'+uid+'_sh.mat', 'wb')
fshared.close()

i = 0
for f in infiles:
  mesg.append('whatever,SHELL,'+ str(i) +','+ f +',\n')
  print 'sending data:', mesg[i]
  cnn.send(mesg[i])
  running_jobs.append(i)
  i += 1

cnn.send('done')

while len(running_jobs) > 0:
  mesg = cnn.recv(8192)
  print 'received data:' + mesg
  print 'removing job:' + mesg.split(':')[0].strip()
  running_jobs.remove(int(mesg.split(':')[0].strip()))

print 'All jobs finished'

cnn.send('kill\n')

cnn.close()
