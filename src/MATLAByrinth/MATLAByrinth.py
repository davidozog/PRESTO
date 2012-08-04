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
import posix_ipc
import mmap
import shmemUtils

DEBUG = True

#MATLAB_BIN = '/usr/local/packages/MATLAB/R2011b/bin/matlab'
MATLAB_BIN = os.environ['MATLAB'] + '/bin/matlab'
CHECKIN_TAG = 1
DO_WORK_TAG = 2
KILL_TAG    = 3
RESULTS_TAG = 4

WORKER_PORT = 11110
MASTER_PORT = 11112

import pickle
def saveobject(obj, filename):
  with open(filename, 'wb') as output:
    pickle.dump(obj, output, pickle.HIGHEST_PROTOCOL)

def launch_matlab(queue, worker_dict):
  # Without GUI:
  p = subprocess.Popen(MATLAB_BIN + ' -nodesktop -nosplash -r \"'+ worker_dict + '\"', shell=True)
  # With GUI:
  #p = subprocess.Popen(MATLAB_BIN + ' -desktop -r \"'+ worker_dict + '\"', shell=True)
  queue.put('running')
  p.communicate()
  # send kill signal to all workers
  q.put('kill')


# Initialize mpi4py:
mpiComm = MPI.COMM_WORLD
size = mpiComm.Get_size()
rank = mpiComm.Get_rank()
name = MPI.Get_processor_name()
srank = str(rank)

print "Process %d of %d launched on %s." % (rank, size, name)

# MASTER PROCESS:
if (rank==0):
  master_status = MPI.Status()
  data = {}
  for i in range(1,size):
    data[i] = mpiComm.recv(source=i, tag=CHECKIN_TAG)

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
            protocol = mesg.split(',')[1].strip()
            if mesg == 'done\n':
              break
            if protocol == 'NETWORK':
              while True:
                try:
                  semaphore = posix_ipc.Semaphore('MATSEM')
                  break
                except:
                  time.sleep(1)
                  continue
              memory = posix_ipc.SharedMemory('MAT2SHM')
              mapfile = mmap.mmap(memory.fd, memory.size)
              os.close(memory.fd)
              semaphore.acquire()
              mesg_len = int(mesg.split()[-1])
              mesg_dat = shmemUtils.read_from_memory(mapfile, mesg_len)
              print mesg_dat
              semaphore.release()
              mesg = mesg + mesg_dat
            # Determine the rank of the next worker:
            if (destination != size-1):
              destination = (destination + 1) % size
            else:
              destination = 1
            # If all workers are busy wait for a result and send to that worker.
            if len(running_jobs) == size-1:
              #TODO: this could be a function:
              if(DEBUG):print running_jobs; print '     master waiting for a free worker...'
              data = mpiComm.recv(source=MPI.ANY_SOURCE, tag=RESULTS_TAG, status=master_status)
              if(DEBUG):print '     GOT extra RESULT:' + data
              running_jobs.remove(master_status.source)
              cnn.sendall(data)
              mpiComm.send(mesg, dest=master_status.source, tag=DO_WORK_TAG)
              running_jobs.append(master_status.source)
              master_status = MPI.Status()
            else:
              mpiComm.send(mesg, dest=destination, tag=DO_WORK_TAG)
              running_jobs.append(destination)
          else:
            continue

        # Wait for job completion 
        while len(running_jobs) > 0:
          #TODO: this could be a function
          if(DEBUG):print running_jobs; print '     master waiting for results...'
          data = mpiComm.recv(source=MPI.ANY_SOURCE, tag=RESULTS_TAG, status=master_status)
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
  mpiComm.send(data, dest=0, tag=CHECKIN_TAG)
  args = [MATLAB_BIN, '-nodesktop', '-nosplash', '-r', 'mworker(\''+name+'\', '+srank+')']  
  #p = subprocess.Popen(args, stdout=subprocess.PIPE)
  p = subprocess.Popen(args, stdout=open('worker'+srank+'.log', 'w'))

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
    mesg = mpiComm.recv(source=0, tag=MPI.ANY_TAG, status=worker_status)
    protocol = mesg.split(',')[1].strip()
    if (worker_status.tag == DO_WORK_TAG):
      if(DEBUG):print 'P'+srank+':(sending): do_work ( ' + mesg + ' ) ' 
      
      # TODO: send byte array to matlab via shmem
      if protocol == 'NETWORK':
        print 'Yet again, you made it'
        mesg_data = mesg.split('\n')[-1].strip()
#        saveobject(mesg_data, r'/home11/ozog/School/MATLAByrinth/mesg_pickle')
        print "worker mesg_data is " + mesg_data

        # TODO: Fix this - why can't I open an already existing shmem / semaphore?
        #memory = posix_ipc.SharedMemory('SHM2MAT', posix_ipc.O_CREX, size=4096)
        #semaphore = posix_ipc.Semaphore('PYSEM', posix_ipc.O_CREX)
        try:
          memory = posix_ipc.SharedMemory('SHM2MAT', posix_ipc.O_CREX, size=4096)
        except:
          memory = posix_ipc.SharedMemory('SHM2MAT')
          memory.unlink()
          memory = posix_ipc.SharedMemory('SHM2MAT', posix_ipc.O_CREX, size=4096)
        try:
          semaphore = posix_ipc.Semaphore('PYSEM', posix_ipc.O_CREX)
        except:
          semaphore = posix_ipc.Semaphore('PYSEM')
          semaphore.unlink()
          semaphore = posix_ipc.Semaphore('PYSEM', posix_ipc.O_CREX)
        mapfile = mmap.mmap(memory.fd, memory.size)
        #memory.close_fd()
        shmemUtils.write_to_memory(mapfile, mesg_data)
        semaphore.release()
        conn.sendall(mesg)
        time.sleep(1)
        #semaphore.acquire()
        #memory.unlink()
        #mapfile.close()
        

      else:
        # Send work message (func_name, 'split_file.mat', 'shared_file.mat')
        conn.sendall(mesg)

      # Wait for results (filename):
      data = conn.recv(1024)
      if(DEBUG):print 'P'+srank+':(received): ' + data
      semaphore.acquire()
      memory.unlink()
      mapfile.close()

      # Send results (filename) back to master:
      mpiComm.send(data, dest=0, tag=RESULTS_TAG)

      # See if the master has decided to die:
      mpiComm.Iprobe(source=0, tag=KILL_TAG, status=worker_status)
      if (worker_status.tag == KILL_TAG):
        if(DEBUG):print 'KILL MESSAGE came from ' + str(worker_status.source)
        kill = mpiComm.recv(source=worker_status.source, tag=worker_status.tag)
    elif (worker_status.tag == KILL_TAG):
      break

# MPI : once rank 0 Matlab is dead, kill the workers
if(DEBUG):print 'kill is first ' + str(kill) + ' on rank ' + srank
#mpiComm.bcast(kill, root=0, tag=KILL_TAG)
if (rank==0):
  sock.close()
  kill = 1
  for i in range(1, size):
    mpiComm.send(kill, dest=i, tag=KILL_TAG)
if(DEBUG):print 'kill is THEN ' + str(kill) + ' on rank ' + srank
if rank != 0:
  s.close()
  if(DEBUG):print 'killing ' + str(p.pid)
  os.kill(p.pid, signal.SIGUSR1)

