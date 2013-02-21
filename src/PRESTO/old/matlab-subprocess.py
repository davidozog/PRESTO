import subprocess
import os
import threading
import time
import signal
import socket
from mpi4py import MPI
import sys

size = MPI.COMM_WORLD.Get_size()
rank = MPI.COMM_WORLD.Get_rank()
name = MPI.Get_processor_name()

sys.stdout.write(
    "Hello, World! I am process %d of %d on %s.\n"
    % (rank, size, name))


HOST = "localhost"
PORT = 11111

def kill_thread(pid):
  time.sleep(12)
  os.kill(pid, signal.SIGUSR1)


args = ['matlab', '-nodesktop', '-nosplash', '-r', 'jserver']
proc = subprocess.Popen(args, stdout=subprocess.PIPE)

t = threading.Thread(target=kill_thread, args=(proc.pid,))
t.start()

#proc.wait()
time.sleep(11)

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

print "send: hey_server\n", 
sock.sendall("hey_server\n")

data = sock.recv(1024)
print "received: " + data

if ( data == "hi client\n" ):
  print "send: bye_server"
  sock.sendall("bye_server\n")
  data = sock.recv(1024)
  print "received: " + data

  if (data == "bye client}\n"):
    sock.close()
    print "Socket closed"

stdout_value = proc.communicate()
#print '\tstdout:', repr(stdout_value)

for line in stdout_value:
  print line
