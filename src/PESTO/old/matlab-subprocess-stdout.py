import subprocess
import os
import threading
import time
import signal
import socket
from mpi4py import MPI
import sys
from Queue import Queue, Empty

FUNAME = sys.argv[1] 
SPLIT_FILE = sys.argv[2]
SHARED_FILE = sys.argv[3]
print 'funcname is ' + FUNAME 
print 'SPLIT_FILE is ' + SPLIT_FILE
print 'SHARED_FILE is ' + SHARED_FILE


def kill_thread(pid):
  time.sleep(2)
  try:
    os.kill(pid, signal.SIGUSR1)
  except:
    print "ERROR: Kill Failed..."

def enqueue_output(out, queue):
  for line in iter(out.readline, b''):
    queue.put(line)
  out.close()

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()
name = MPI.Get_processor_name()
srank = str(rank)

print "I am process %d of %d on %s.\n" % (rank, size, name)

HOST = socket.gethostname()
PORT = 11111


args = ['matlab', '-nodesktop', '-nosplash', '-r', 'jserver(\''+name+'\', '+srank+', \''+FUNAME+'\')']  
#proc = subprocess.Popen(args, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT)
#fsin, fsout_and_fserr = (proc.stdout, proc.stdin)



p = subprocess.Popen(args, stdout=subprocess.PIPE, bufsize=1024)
q = Queue()
time.sleep(10) # Give Matlab some time to start up

t = threading.Thread(target=enqueue_output, args=(p.stdout, q))
t.daemon = True # thread dies with the program
t.start()

#k = threading.Thread(target=kill_thread, args=(p.pid,))
#k.start()

# read line without blocking
while True:
  try:  line = q.get_nowait() # or q.get(timeout=.1)
  except Empty:
    #print('no output')
    time.sleep(1)
    break
  else: # got line
    sys.stdout.write(line)
    #time.sleep(1)
    

#import pdb; pdb.set_trace()
#time.sleep(10)
#stdout_value = proc.communicate()

#print '\tstdout:', repr(stdout_value)


#proc.wait()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

print 'P'+srank+':(sending): do_work\n', 
sock.sendall('do_work___\n')

data = sock.recv(1024)
print 'P'+srank+':(received): ' + data

data = comm.gather(data, root=0)
print 'RANK ' + srank + ' MADE IT OUT OF GATHER.'
if rank == 0:
  for i in range(size):
    print 'data['+str(i)+'] is ' + data[i]
    #assert data[i] == str(i) + '\n'
  print 'success\n'
else:
  assert data is None

print 'P'+srank+'(sending): bye_matlab'
sock.sendall('bye_matlab\n')
data = sock.recv(1024)
print 'P'+srank+':(received): ' + data

if (data == 'bye master}\n'):
  sock.close()
  print 'Socket closed'




k = threading.Thread(target=kill_thread, args=(p.pid,))
k.start()
