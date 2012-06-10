from mpi4py import MPI
import subprocess
import sys
import os
import signal
import socket
import threading
import time
from Queue import Queue, Empty
import struct

DEBUG = False

MATLAB_BIN = '/usr/local/packages/MATLAB/R2011b/bin/matlab'
CHECKIN_TAG = 1
DO_WORK_TAG = 2
KILL_TAG    = 3
RESULTS_TAG = 4

WORKER_PORT = 11110
MASTER_PORT = 11112

def launch_matlab(queue, worker_dict):
  # Without GUI:
  p = subprocess.Popen(MATLAB_BIN + ' -nodesktop -nosplash -r \"'+ worker_dict + '\"', shell=True)
  # With GUI:
  #p = subprocess.Popen(MATLAB_BIN + ' -desktop -r \"'+ worker_dict + '\"', shell=True)
  queue.put('running')
  p.communicate()
  # send kill signal to all workers
  q.put('kill')


comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()
name = MPI.Get_processor_name()
srank = str(rank)

print "Process %d of %d launched on %s." % (rank, size, name)

# MASTER CONTROL:
if (rank==0):
  master_status = MPI.Status()
  data = {}
  for i in range(1,size):
    data[i] = comm.recv(source=i, tag=CHECKIN_TAG)

  worker_dict = 'matlabyrinth_workers = struct(\'hosts\', {'
  for i in range(1,size):
    worker_dict = worker_dict + '\'' + data[i]['name'] + '\', '
  worker_dict = worker_dict + '}, \'ranks\', {'
  for i in range(1,size):
    worker_dict = worker_dict + str(data[i]['rank']) + ', '
  worker_dict = worker_dict + '})'

  q = Queue()
  t = threading.Thread(target=launch_matlab, args=(q, worker_dict))
  t.start()

  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  #sock.settimeout(10)
  #sock.setblocking(0)  # Sets sockets to non-blocking
  sock.bind(('localhost', MASTER_PORT))
  sock.listen(1)

  while True:
    try:  
      line = q.get_nowait() # or q.get(timeout=.1)
    except Empty:
      time.sleep(1)
    else: # got line
      if line == 'running':
        while True:
          try:
            cnn, addr = sock.accept()
            if(DEBUG):print '     Master Connection :', addr
            break
          except:
            continue

        # Get jobs from 'send_jobs_to_workers' call
        mesg = ''
        mesg_q = Queue()
        destination = 0
        running_jobs = []   # rank of the workers with running jobs
        while mesg!='kill':
          if(DEBUG):print '     master waiting for work...'
          if mesg_q.qsize() > 0:
            mesg = mesg_q.get() + '\n'
            fromq = True
          else:
            mesg = cnn.recv(1024)
            fromq = False
          mesg_split = filter(None, mesg.split('\n'))
          for m in mesg_split:
            if len(m) > 0 and (m) != mesg:
              mesg_q.put(m)
          if mesg_q.qsize > 0 and not fromq:
            mesg = mesg_q.get() + '\n'
          if (len(mesg) > 0):
            if(DEBUG):print '     master message is ' + mesg 
            if mesg == 'done\n':
              break
            if (destination != size-1):
              destination = (destination + 1) % size
            else:
              destination = 1
            if len(running_jobs) == size-1:
              #TODO: this could be a function:
              if(DEBUG):print running_jobs; print '     master waiting for a free worker...'
              data = comm.recv(source=MPI.ANY_SOURCE, tag=RESULTS_TAG, status=master_status)
              if(DEBUG):print '     GOT extra RESULT:' + data
              running_jobs.remove(master_status.source)
              cnn.sendall(data)
              comm.send(mesg, dest=master_status.source, tag=DO_WORK_TAG)
              running_jobs.append(master_status.source)
              master_status = MPI.Status()
            else:
              comm.send(mesg, dest=destination, tag=DO_WORK_TAG)
              running_jobs.append(destination)
          else:
            continue

        # Wait for job completion 
        while len(running_jobs) > 0:
          #TODO: this could be a function
          if(DEBUG):print running_jobs; print '     master waiting for results...'
          data = comm.recv(source=MPI.ANY_SOURCE, tag=RESULTS_TAG, status=master_status)
          if(DEBUG):print 'GOT RESULT:' + data
          running_jobs.remove(master_status.source)
          master_status = MPI.Status()
          cnn.sendall(data)

        if(DEBUG):print 'All workers have sent back results.'

        cnn.close()

        q.put('running')

      if line == 'kill':
        print 'kill signal'
        kill = 1
        break

# WORKER CONTROL:
else: 
  data = {'name': name, 'rank': rank}
  comm.send(data, dest=0, tag=CHECKIN_TAG)
  args = [MATLAB_BIN, '-nodesktop', '-nosplash', '-r', 'mworker(\''+name+'\', '+srank+')']  
  p = subprocess.Popen(args, stdout=subprocess.PIPE)

  # Wait for "alive" message from each workers
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  s.bind((name, WORKER_PORT))
  s.listen(1)
  conn, addr = s.accept()
  print 'Worker connected: ', addr
  alive = conn.recv(1024)
  kill = None

  # Wait for a message from the Master to do work
  while (kill != 1):
    worker_status = MPI.Status()
    mesg = comm.recv(source=0, tag=MPI.ANY_TAG, status=worker_status)
    if (worker_status.tag == DO_WORK_TAG):
      if(DEBUG):print 'P'+srank+':(sending): do_work ( ' + mesg + ' ) ' 

      # Send work message (func_name, 'split_file.mat', 'shared_file.mat')
      conn.sendall(mesg)

      # Wait for results (filename):
      data = conn.recv(1024)
      if(DEBUG):print 'P'+srank+':(received): ' + data

      # Send results (filename) back to master:
      comm.send(data, dest=0, tag=RESULTS_TAG)

      # See if the master has decided to die:
      comm.Iprobe(source=0, tag=KILL_TAG, status=worker_status)
      if (worker_status.tag == KILL_TAG):
        if(DEBUG):print 'KILL MESSAGE came from ' + str(worker_status.source)
        kill = comm.recv(source=worker_status.source, tag=worker_status.tag)
    elif (worker_status.tag == KILL_TAG):
      break

# MPI : once rank 0 Matlab is dead, kill the workers
if(DEBUG):print 'kill is first ' + str(kill) + ' on rank ' + srank
#comm.bcast(kill, root=0, tag=KILL_TAG)
if (rank==0):
  sock.close()
  kill = 1
  for i in range(1, size):
    comm.send(kill, dest=i, tag=KILL_TAG)
if(DEBUG):print 'kill is THEN ' + str(kill) + ' on rank ' + srank
if rank != 0:
  s.close()
  if(DEBUG):print 'killing ' + str(p.pid)
  os.kill(p.pid, signal.SIGUSR1)

