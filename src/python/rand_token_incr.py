import subprocess
import socket
import sys

print 'DON\'T FORGET TO HARD-CODE THE NUMBER OF WORKERS'

NUM_WORKERS = 2
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

mesg = []
running_jobs = []

for i in range(0,NUM_WORKERS):
  mesg.append('dummy_function,DAG,'+ str(i+1) +','+ str(i+1) +',0,\n')
  cnn.send(mesg[i])
  running_jobs.append(i+1)

cnn.send('done')

print '\n'
while len(running_jobs) > 0:
  mesg = cnn.recv(8192)
  mesg_split = mesg.split('\n')
  for m in mesg_split:
    if len(m) > 0:
      print 'received token:' + m.split(':')[0] + ' with value: ' + m.split(':')[2] 
      running_jobs.remove(int(m.split(':')[0].strip()))
print '\n'

cnn.send('kill\n')

cnn.close()
