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
import sysv_ipc
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

JQKEY = 37408
RQKEY = 48963

def launch_matlab(queue, worker_dict):
  # Without GUI:
  p = subprocess.Popen(MATLAB_BIN + ' -nodesktop -nosplash -r \"'+ 
                       worker_dict + '\"', shell=True)
  # With GUI:
  #p = subprocess.Popen(MATLAB_BIN + ' -desktop -r \"'+ worker_dict + 
  #                     '\"', shell=True)
  queue.put('running')
  p.communicate()
  # send kill signal to all workers
  q.put('kill')

def master_shmem_initiailization():
  #try:
  #  semaphore = posix_ipc.Semaphore('MATSEM')
  #  semaphore.unlink()
  #except:
  #  pass 
  #try: 
  #  memory = posix_ipc.SharedMemory('MAT2SHM')
  #  memory.unlink()
  #except:
  #  pass
  try: 
    jq = sysv_ipc.MessageQueue(JQKEY)
    jq.remove()
  except:
    pass
  try: 
    # Results Queue (rq)
    rq = sysv_ipc.MessageQueue(RQKEY)
    rq.remove()
  except:
    pass
 
def worker_shmem_initiailization():
  try:
    memory = posix_ipc.SharedMemory('SHM2MAT')
    memory.unlink()
  except:
    pass
  try:
    semaphore = posix_ipc.Semaphore('PYSEM')
    semaphore.unlink()
  except:
    pass

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

  master_shmem_initiailization()
  rq = sysv_ipc.MessageQueue(RQKEY, sysv_ipc.IPC_CREX)
  jq = sysv_ipc.MessageQueue(JQKEY, sysv_ipc.IPC_CREX)

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
        #import pdb; pdb.set_trace()
        while mesg!='kill':
          if(DEBUG):print '     master waiting for work...'
          if mesg_q.qsize() > 0:
            mesg = mesg_q.get() + '\n'
            fromq = True
          else:
            mesg = cnn.recv(1024)
            fromq = False
          # why is 'filter' here?  TODO:
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
            protocol = mesg.split(',')[1].strip()
            if protocol == 'NETWORK':
              #while True:
              #  try:
              #    jq = sysv_ipc.MessageQueue(JQKEY)
              #    semaphore = posix_ipc.Semaphore('MATSEM')
              #    break
              #  except:
              #    time.sleep(1)
              #    continue
              mesg_len = int(mesg.split()[-1])
              #semaphore.acquire()
              #memory = posix_ipc.SharedMemory('MAT2SHM')
              #mapfile = mmap.mmap(memory.fd, memory.size)
              #os.close(memory.fd)
              #mesg_dat = shmemUtils.read_from_memory(mapfile, mesg_len)
              #print mesg_dat
              #semaphore.release()

              #import pdb; pdb.set_trace()

              (mesg_dat, mesg_size) = jq.receive()
              mesg_dat = mesg_dat[:mesg_size]

              mesg = mesg.strip() + ':' + mesg_dat + '\n'

            # Determine the rank of the next worker:
            if (destination != size-1):
              destination = (destination + 1) % size
            else:
              destination = 1
            # If all workers are busy wait for a result and 
            # send new job to that worker.
            if len(running_jobs) == size-1:
              #TODO: this could be a function:
              if(DEBUG):print running_jobs; print '     master waiting for a free worker...'
              data = mpiComm.recv(source=MPI.ANY_SOURCE, tag=RESULTS_TAG, status=master_status)
              if(DEBUG):print '     GOT extra RESULT:' + data
              running_jobs.remove(master_status.source)

              if protocol == 'NETWORK':
                result_mesg = ':'.join(data.split(':')[:-1]) + '\n'
                cnn.sendall(result_mesg)

                mesg_data = data.split(':')[-1]
                mesg_jobid = int(data.split(':')[-2])

                rq.send(mesg_data, type=mesg_jobid)
      
              else:
                cnn.sendall(data)

              mpiComm.send(mesg, dest=master_status.source, 
                           tag=DO_WORK_TAG)
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

          # TODO - send data to shmem - using a new shmem 
          # segment with a message queue
          if protocol == 'NETWORK':
            result_mesg = ':'.join(data.split(':')[:-1]) + '\n'
            cnn.sendall(result_mesg)

            mesg_data = data.split(':')[-1]
            mesg_jobid = int(data.split(':')[-2])

            rq.send(mesg_data, type=mesg_jobid)
      
          else:
            cnn.sendall(data)

        if(DEBUG):print 'All workers have sent back results.'

        cnn.close()
        #jq.remove()

        q.put('running')

      if line == 'kill':
        print 'kill signal'
        kill = 1
        rq.remove()
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

  worker_shmem_initiailization()
  memory = posix_ipc.SharedMemory('SHM2MAT', posix_ipc.O_CREX, size=4096)
  semaphore = posix_ipc.Semaphore('PYSEM', posix_ipc.O_CREX)

  # Wait for a message from the Master to do work
  while (kill != 1):
    worker_status = MPI.Status()
    mesg = mpiComm.recv(source=0, tag=MPI.ANY_TAG, status=worker_status)
    protocol = mesg.split(',')[1].strip()
    if (worker_status.tag == DO_WORK_TAG):
      if(DEBUG):print 'P'+srank+':(sending): do_work ( ' + mesg + ' ) ' 
      
      # Send byte array to Matlab worker via shmem
      if protocol == 'NETWORK':
        mesg_data = mesg.split(':')[-1].strip()
        print "worker mesg_data is " + mesg_data

        memory = posix_ipc.SharedMemory('SHM2MAT')
        semaphore = posix_ipc.Semaphore('PYSEM')
        mapfile = mmap.mmap(memory.fd, memory.size)
        shmemUtils.write_to_memory(mapfile, mesg_data)
        semaphore.release()
        conn.sendall(mesg)
        
      else:
        # Send work message (func_name,'split_file.mat','shared_file.mat')
        conn.sendall(mesg)

      # Wait for results (filename):
      data = conn.recv(1024)
      if(DEBUG):print 'P'+srank+':(received): ' + data

      if protocol == 'NETWORK':
        while True:
          try:
            semaphore = posix_ipc.Semaphore('MATSEM')
            break
          except:
            #time.sleep(1)
            continue
        mesg_len = int(data.split(':')[-2])
        jobid = int(data.split(':')[-1])
        semaphore.acquire()
        memory = posix_ipc.SharedMemory('MAT2SHM')
        mapfile = mmap.mmap(memory.fd, memory.size)
        os.close(memory.fd)
        shm_data = shmemUtils.read_from_memory(mapfile, mesg_len)
        semaphore.release()
        data = data.strip() + ':' + shm_data

      # Send results (filename) back to master:
      mpiComm.send(data, dest=0, tag=RESULTS_TAG)

      # See if the master has decided to die:
      mpiComm.Iprobe(source=0, tag=KILL_TAG, status=worker_status)
      if (worker_status.tag == KILL_TAG):
        if(DEBUG):print 'KILL MESSAGE came from ' + str(worker_status.source)
        kill = mpiComm.recv(source=worker_status.source, tag=worker_status.tag)
    elif (worker_status.tag == KILL_TAG):
      if protocol == 'NETWORK':
        semaphore.acquire()
        memory.unlink()
        mapfile.close()
      break

# MPI : once rank 0 Matlab is dead, kill the workers
if(DEBUG):print 'kill is first ' + str(kill) + ' on rank ' + srank
#mpiComm.bcast(kill, root=0, tag=KILL_TAG)
if (rank==0):
  jq.remove()
  sock.close()
  kill = 1
  for i in range(1, size):
    mpiComm.send(kill, dest=i, tag=KILL_TAG)
if(DEBUG):print 'kill is THEN ' + str(kill) + ' on rank ' + srank
if rank != 0:
  s.close()
  if(DEBUG):print 'killing ' + str(p.pid)
  os.kill(p.pid, signal.SIGUSR1)

