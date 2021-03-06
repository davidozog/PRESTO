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
from multiprocessing import Process, Pipe

DEBUG = True

#MATLAB_BIN = '/usr/local/packages/MATLAB/R2011b/bin/matlab'
MATLAB_BIN = os.environ['MATLAB'] + '/bin/matlab'
PRESTO_DIR = os.environ['PRESTO']
CHECKIN_TAG = 1
DO_WORK_TAG = 2
KILL_TAG    = 3
RESULTS_TAG = 4

WORKER_PORT = 11110
MASTER_PORT = 11112

TOKEN_MAX = 1000

JQKEY = 37408
RQKEY = 48963

TMPFS_PATH = '/dev/shm/'

# Set to True if using "NETWORK" mode
messageQueue = False

def launch_matlab(queue, worker_dict):
  # Without GUI:
  presto_mpath = os.path.join(PRESTO_DIR, 'presto_setpath.m')

  p = subprocess.Popen(MATLAB_BIN + ' -nodesktop -nosplash -r \"run ' + presto_mpath + ';' + worker_dict + '\"', shell=True)
  #p = subprocess.Popen(MATLAB_BIN + ' -nodesktop -nosplash -r \"run ' + presto_mpath + ';' + worker_dict + '\"', stdout=open('worker0.log', 'w'), stderr=open('worker0.err', 'w'), shell=True)
  # With GUI:
  #p = subprocess.Popen(MATLAB_BIN + ' -desktop -r \"'+ worker_dict + 
  #                     '\"', shell=True)
  queue.put('running')
  p.communicate()
  # send kill signal to all workers
  q.put('kill')

def launch_java(queue, worker_dict, java_app):
  p = subprocess.Popen('java ' + java_app, shell=True)
  queue.put('running')
  p.communicate()
  # send kill signal to all workers
  q.put('kill')

def launch_python(queue, worker_dict, pyprog):
  #p = subprocess.Popen(pyprog, stdout=open('worker0.log', 'w'), stderr=open('worker0.err', 'w'), shell=True)
  p = subprocess.Popen(pyprog, shell=True)
  queue.put('running')
  p.communicate()
  # send kill signal to all workers
  q.put('kill')

def launch_cpp(queue, worker_dict, cppProg):
  #p = subprocess.Popen(pyprog, stdout=open('worker0.log', 'w'), stderr=open('worker0.err', 'w'), shell=True)
  print 'launching: ' + cppProg
  p = subprocess.Popen(cppProg, shell=True)
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

# Get the user id for the file objects
def get_uid():
  uid = subprocess.check_output('id')
  uid = uid.split()[0]
  lft = uid.index('=') + 1
  rht = uid.index('(')
  uid = uid[lft:rht]
  return uid

def waitOnResults(conn, mpiComm, protocol):
  data = conn.recv(512)
  if(DEBUG):print 'W'+srank+':(finished/received): ' + data 
  if len(data.strip()) > 1:

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

    elif protocol == 'DAG':
      if(DEBUG): print 'DAG results mesg is:' + data

    elif protocol == 'SHELL':
      if(DEBUG): print 'results mesg is:' + data
      jobid = data.split(':')[0]
      if(DEBUG): print 'jobid is:' + jobid

    elif protocol == 'TMPFS':
      if(DEBUG): print 'results file is' + data
      results_file = open(data.strip(), 'rb')
      idx = data.find('r')
      jobid = data[idx+1:data.find('.',idx+1)]
      data = jobid + ':' + results_file.read()

    # Send results (filename) back to master:
    mpiComm.send(data, dest=0, tag=RESULTS_TAG)

    # See if the master has decided to die:
    #mpiComm.Iprobe(source=0, tag=KILL_TAG, status=worker_status)
    #if (worker_status.tag == KILL_TAG):
    #  if(DEBUG):print 'KILL MESSAGE came from ' + str(worker_status.source)
    #  kill = mpiComm.recv(source=worker_status.source, tag=worker_status.tag)

def Rvis(conn):
  print conn.recv()
  #p = subprocess.Popen('R --no-save', stdin=subprocess.PIPE, shell=True)
  #p = subprocess.Popen('matlab -nodesktop -nosplash', stdin=subprocess.PIPE, shell=True)
  #p = subproCess.Popen('R --interactive', stdin=subprocess.PIPE, shell=True)
  #p = subprocess.Popen('R --interactive', shell=True)
  #p = subprocess.Popen('R --no-save < socket.R', stdout=subprocess.PIPE, shell=True)
  p = subprocess.Popen('R --no-save --interactive < socket.R', shell=True)

  #out, err = p.communicate('x=rnorm(1000,mean=0,sd=1); hist(x)')
  #p.wait()
  #out, err = p.communicate('x=norm(1000); hist(x);')
  #out, err = p.communicate('y=norm(10)')
  p.communicate()
  #print out

def analyzeResults(mesg):
  p = subprocess.check_output("./analyzeResults " + mesg, shell=True)  
  return p.strip()



# Initialize mpi4py:
mpiComm = MPI.COMM_WORLD
size = mpiComm.Get_size()
rank = mpiComm.Get_rank()
name = MPI.Get_processor_name()
srank = str(rank)
interface = sys.argv[1]

print "Process %d of %d launched on %s." % (rank, size-1, name)


# MASTER PROCESS:
if (rank==0):
  master_status = MPI.Status()
  data = {}
  for i in range(1,size):
    data[i] = mpiComm.recv(source=i, tag=CHECKIN_TAG)

  worker_dict = 'presto_workers = struct(\'hosts\', {'
  for i in range(1,size):
    worker_dict = worker_dict + '\'' + data[i]['name'] + '\', '
  worker_dict = worker_dict + '}, \'ranks\', {'
  for i in range(1,size):
    worker_dict = worker_dict + str(data[i]['rank']) + ', '
  worker_dict = worker_dict + '})'

  q = Queue()
  if interface == 'matlab':
    print 'sysv is:' + str(len(sys.argv))
    if len(sys.argv) == 3:
      worker_dict = worker_dict + ';' + sys.argv[2]
      print 'worker_dict:' + worker_dict
    t = threading.Thread(target=launch_matlab, args=(q, worker_dict))
  elif interface == 'java':
    java_app = sys.argv[2]
    t = threading.Thread(target=launch_java, args=(q, worker_dict, java_app))
  elif interface == 'python':
    py_app = sys.argv[2]
    t = threading.Thread(target=launch_python, args=(q, worker_dict, "python " + py_app))
  elif interface == 'cpp' or interface == 'cppvis':
    cpp_app = sys.argv[2]
    if cpp_app.find('presto_for_') >= 0:
      cpp_app = sys.argv[2] + ' ' + sys.argv[3]
    t = threading.Thread(target=launch_cpp, args=(q, worker_dict, cpp_app))

  t.start()

  master_shmem_initiailization()
  if messageQueue:
    rq = sysv_ipc.MessageQueue(RQKEY, sysv_ipc.IPC_CREX)
    jq = sysv_ipc.MessageQueue(JQKEY, sysv_ipc.IPC_CREX)

  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  #sock.settimeout(10)
  #sock.setblocking(0)  # Sets sockets to non-blocking
  sock.bind(('localhost', MASTER_PORT))
  sock.listen(1)
  firstrun = True
  protocol = 'DUMMY'
  dag_d = {}
  dag_d_num = 0


  while True:
    try:  
      line = q.get_nowait() # or q.get(timeout=.1)
    except Empty:
      time.sleep(1)
    else: # got line
      if line == 'running':
        if firstrun:
          while True:
            try:
              cnn, addr = sock.accept()
              if(DEBUG):print '     Master Connection :', addr
              break
            except:
              print "no connection"
              continue

        # Get jobs from 'send_jobs_to_workers' call
        if protocol!='DAG' or firstrun:
          mesg = ''
          mesg_q = Queue()
          running_jobs = []   # rank of the workers with running jobs
        if interface != 'cppvis':
          destination = 0
        else:
          destination = 1
        while mesg!='kill':
          if(DEBUG):print '     master waiting for work...'
          mesg = ''
          availableWorker = False
          if mesg_q.qsize() > 0:
            mesg = mesg_q.get() + '\n'
            fromq = True
          else:
            mesg = cnn.recv(8192)
            fromq = False
          mesg_split = filter(None, mesg.split('\n'))
          if not fromq:
            for m in mesg_split:
              if len(m) > 0 and (m) != mesg and m is not None:
                mesg_q.put(m)
          if mesg_q.qsize() > 0 and not fromq:
            mesg = mesg_q.get() + '\n'
          if (len(mesg) > 0):
            if(DEBUG):print '     master message is ' + mesg 
            if mesg == 'done\n' or mesg == 'kill\n':
              #TODO: reset 'persistent' flag here, I think
              if(DEBUG):print "GOT DONE"
              break
            protocol = mesg.split(',')[1].strip()
            if protocol == 'NETWORK':
              mesg_len = int(mesg.split()[-1])
              (mesg_dat, mesg_size) = jq.receive()
              mesg_dat = mesg_dat[:mesg_size]

              mesg = mesg.strip() + ':' + mesg_dat + '\n'

            elif protocol == 'DAG':
              if(DEBUG):print 'DAG msg is ' + mesg
              tokenid = mesg.split(',')[2].strip()
              mesg_dat = mesg.split(',')[4].strip()
              dag_destination = mesg.split(',')[3].strip()

            elif protocol == 'TMPFS' or protocol == 'SHELL':
              uid = get_uid()
              if(DEBUG):print 'TMPFS file is ' + mesg.split(',')[3].strip()
              tmpfs_file = open(mesg.split(',')[3].strip(), 'rb')
              mesg_dat = tmpfs_file.read()
              # TODO: Just broadcast shared data once per job pool...
              # TODO: And/or add flag to use persistent shared data on worker side
              tmpfs_file.close()

            # Determine the rank of the next worker:
            if protocol != 'DAG':
              if (destination != size-1):
                destination = (destination + 1) % size
              elif interface != 'cppvis':
                destination = 1
              else:
                destination = 2

            else:
              destination = int(dag_destination)
            
            # If all workers are busy wait for a result and 
            # send new job to that worker.
            if len(running_jobs) == size-1:
              #print str(len(running_jobs)) + ' JOBS ARE RUNNING'
              if(DEBUG): 
                print 'master waiting for a free worker...'

              data = mpiComm.recv(source=MPI.ANY_SOURCE, \
                                  tag=RESULTS_TAG,       \
                                  status=master_status)

              if(DEBUG):print 'GOT EXTRA RESULT:' 
              if protocol != 'DAG':
                running_jobs.remove(master_status.source)

              if protocol == 'NETWORK':
                result_mesg = ':'.join(data.split(':')[:-1]) + '\n'

                cnn.sendall(result_mesg)

                mesg_data = data.split(':')[-1]
                mesg_jobid = int(data.split(':')[-2])

                rq.send(mesg_data, type=mesg_jobid)

              elif protocol == 'DAG':
                jobid = data.split(':')[0]
                dest_task = data.split(':')[1]
                token_value = data.split(':')[2]
                running_jobs.remove(jobid)
                if(DEBUG):print 'jobid:' + jobid
                if(DEBUG):print 'token_value:' + token_value
                if int(token_value) < TOKEN_MAX:
                  mesg_q.put('NEW_whatever,DAG,'+ jobid +','+ dest_task +','+ token_value +',\n')
                else:
                  cnn.sendall(data+'\n')
                #print 'token #'+ jobid + ' : ' + token_value

              elif protocol == 'SHELL':
                jobid = data.split(':')[0]
                results_filename = data.split(':')[1]
                if(DEBUG):print 'jobid:' + jobid
                if(DEBUG):print 'results_fname:' + results_filename
                cnn.sendall(data)

              elif protocol == 'TMPFS':
                jobid = data[:data.find(':')]
                #print '   JOBID: ' + jobid + ' FINISHED'
                results_file = open(TMPFS_PATH +'.'+ uid + '_r' + jobid + '.mat', 'wb')
                results_file.write(data[data.find(':')+1:])
                results_file.close()
                if interface == 'cppvis':
                  mpiComm.send(results_file.name, dest=1, tag=DO_WORK_TAG)
                cnn.sendall(results_file.name + '\n')
                #TODO: make sure mesg doesn't contain shared data, and 
                #      transmits 'persist' or something like that
      
              else:
                cnn.sendall(data)

              mesg = mesg.strip() + ':::::' + mesg_dat + ':::::persist\n'

              availableWorker = True
              #mpiComm.send(mesg, dest=master_status.source, tag=DO_WORK_TAG)
              #running_jobs.append(master_status.source)
              #master_status = MPI.Status()

            elif protocol == 'TMPFS' or protocol == 'SHELL':
              tmpfs_filepath_shared = TMPFS_PATH + '.' + uid + '_sh.mat'
              tmpfs_file_shared = open(tmpfs_filepath_shared, 'rb')
              mesg_dat_shared = tmpfs_file_shared.read()
              mesg = mesg.strip() + ':::::' + mesg_dat + ':::::' + mesg_dat_shared + '\n'
              tmpfs_file_shared.close()
          
            if protocol != 'DAG':
              if availableWorker:
                mpiComm.send(mesg, dest=master_status.source, tag=DO_WORK_TAG)
                running_jobs.append(master_status.source)
                master_status = MPI.Status()
              else:
                mpiComm.send(mesg, dest=destination, tag=DO_WORK_TAG)
                running_jobs.append(destination)
            else:
              if destination in dag_d:
                dag_d[destination].append(mesg)
              else:
                dag_d[destination] = [mesg] 
              dag_d_num += 1

              if dag_d_num == size-1:
                for k,v in dag_d.iteritems():
                  destination = int(v[0].split(',')[3].strip())
                  mpiComm.send(v, dest=destination, tag=DO_WORK_TAG)
                  #mpiComm.Isend(mesg, dest=destination, tag=DO_WORK_TAG)
                  for item in v:
                    tokenid = item.split(',')[2].strip()
                    running_jobs.append(tokenid)
                dag_d.clear()
                dag_d_num = 0
                mesg_q.put('done')

          else:
            continue

        # Wait for job completion 
        while len(running_jobs) > 0:
          #print str(len(running_jobs)) + ' JOBS ARE RUNNING'
          if(DEBUG): print 'master waiting for results...'
          data = mpiComm.recv(source=MPI.ANY_SOURCE, tag=RESULTS_TAG, status=master_status)
          #if(DEBUG):print 'GOT RESULT:' + data
          if(DEBUG):print 'GOT RESULT'

          if protocol != 'DAG':
            running_jobs.remove(master_status.source)
  
          master_status = MPI.Status()

          if protocol == 'NETWORK':
            result_mesg = ':'.join(data.split(':')[:0]) + '\n'
            cnn.sendall(result_mesg)

            mesg_data = data.split(':')[-1]
            mesg_jobid = int(data.split(':')[-2])

            rq.send(mesg_data, type=mesg_jobid)

          elif protocol == 'DAG':
            jobid = data.split(':')[0]
            dest_task = data.split(':')[1]
            token_value = data.split(':')[2]
            running_jobs.remove(jobid)
            if(DEBUG):print 'jobid:' + jobid
            if(DEBUG):print 'token_value:' + token_value
            if int(token_value) < TOKEN_MAX:
              mesg_q.put('NEW_whatever,DAG,'+ jobid +','+ dest_task +','+ token_value +',\n')
            else:
              cnn.sendall(data+'\n')
            #print 'token #'+ jobid + ' : ' + token_value

          elif protocol == 'SHELL':
            jobid = data.split(':')[0]
            results_filename = data.split(':')[1]
            if(DEBUG):print 'jobid:' + jobid
            if(DEBUG):print 'results_fname:' + results_filename
            cnn.sendall(data)

          elif protocol == 'TMPFS':
            jobid = data[:data.find(':')]
            #print '   JOBID: ' + jobid + ' FINISHED'
            results_file = open(TMPFS_PATH + '.' + uid + '_r' + jobid + '.mat', 'wb')
            results_file.write(data[data.find(':')+1:])
            results_file.close()
            if interface == 'cppvis':
              mpiComm.send(results_file.name, dest=1, tag=DO_WORK_TAG)
            cnn.sendall(results_file.name + '\n')
      
          else:
            cnn.sendall(data)

        if(DEBUG):print 'All workers have sent back results.'

        #cnn.close()
        #jq.remove()

        q.put('running')

        if interface != 'matlab':
          firstrun = False

      if mesg == 'kill\n':
        print 'kill signal'
        cnn.close()
        kill = 1
        if not firstrun:
          if protocol == 'NETWORK':
            rq.remove()
            jq.remove()
        for i in range(1,size):
          mpiComm.send(",", dest=i, tag=KILL_TAG)
        
        break

# WORKER CONTROL:
elif interface != 'cppvis' or rank != 1: 
  data = {'name': name, 'rank': rank}
  mpiComm.send(data, dest=0, tag=CHECKIN_TAG)
  #args = [MATLAB_BIN, '-nodesktop', '-nosplash', '-r', 'mworker(\''+name+'\', '+srank+')']  
  #p = subprocess.Popen(args, stdout=subprocess.PIPE)

  if interface == 'matlab':
    presto_mpath = os.path.join(PRESTO_DIR, 'presto_setpath.m')
    cmd = MATLAB_BIN + ' -nosplash -r "run ' + presto_mpath + '; mworker(\''+name+'\', '+srank+')" -nodesktop'
  elif interface == 'java':
    cmd = "java Worker " + name + " " + srank
  elif interface == 'python':
    cmd = 'python ' + os.path.join(PRESTO_DIR, 'src/python/py_worker.py') + ' ' +  name + ' ' + srank + ' ' + str(size)
  elif interface == 'cpp' or 'cppvis':
    cmd = os.path.join(PRESTO_DIR, 'src/cpp/cppWorker') + ' ' +  name + ' ' + srank + ' ' + str(size)
  if(DEBUG):print "cmd is: " + cmd
  p = subprocess.Popen(cmd, stdout=open('worker'+srank+'.log', 'w'), stderr=open('worker'+srank+'.err', 'w'), shell=True)

  # Wait for "alive" message from each workers
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  s.bind((name, WORKER_PORT))
  s.listen(1)
  conn, addr = s.accept()
  print 'Worker connected: ', addr
  alive = conn.recv(512)
  kill = None

  worker_shmem_initiailization()
  #memory = posix_ipc.SharedMemory('SHM2MAT', posix_ipc.O_CREX, size=8192)
  #semaphore = posix_ipc.Semaphore('PYSEM', posix_ipc.O_CREX)

  # Wait for a message from the Master to do work
  while (kill != 1):
    worker_status = MPI.Status()
    mesg = mpiComm.recv(source=0, tag=MPI.ANY_TAG, status=worker_status)
    try:
      protocol = mesg.split(',')[1].strip()
    except:
      protocol = mesg[0].split(',')[1].strip()
      
    if (worker_status.tag == DO_WORK_TAG):

      need_results = True

      # Send byte array to Matlab worker via shmem
      if protocol == 'NETWORK':
        mesg_data = mesg.split(':')[-1].strip()
        if(DEBUG):print "worker mesg_data is " + mesg_data

        memory = posix_ipc.SharedMemory('SHM2MAT')
        semaphore = posix_ipc.Semaphore('PYSEM')
        mapfile = mmap.mmap(memory.fd, memory.size)
        shmemUtils.write_to_memory(mapfile, mesg_data)
        semaphore.release()
        conn.sendall(mesg)

      elif protocol == 'DAG':
        if len(mesg) == 1:
          mesg = mesg[0]
          mesg = mesg.split(':')[0].strip()
          if(DEBUG):print 'DAG worker mesg:' + mesg
          conn.sendall(mesg)
        else:
          for i in range(0,len(mesg)):
            job_mesg = mesg[i].split(':')[0].strip()
            if(DEBUG):print 'DAG worker job_mesg:' + job_mesg 
            conn.sendall(job_mesg)
            waitOnResults(conn, mpiComm, protocol)
          need_results = False

      elif protocol == 'TMPFS' or protocol == 'SHELL':
        worker_tmpfs_file = open(mesg.split(',')[3].strip(), 'wb')
        mesg_data_split = mesg.split(':::::')
        mesg = mesg.split(':')[0]
        if(DEBUG):print 'TMPFS worker mesg:' + mesg
        worker_tmpfs_file.write(mesg_data_split[1])
        worker_tmpfs_file.close()
        if mesg_data_split[2] != 'persist\n':
          uid = get_uid()
          worker_tmpfs_filepath_shared = TMPFS_PATH + '.' + uid + '_sh.mat'
          worker_tmpfs_file_shared = open(worker_tmpfs_filepath_shared, 'wb')
          if mesg_data_split[2][0] == ':':
            # This is just a fix to a weird bug:
            worker_tmpfs_file_shared.write(mesg_data_split[2][1:])
          else:
            worker_tmpfs_file_shared.write(mesg_data_split[2])
          worker_tmpfs_file_shared.close()
        else:
          mesg = mesg + ", persist"  
        mesg = mesg + '\n'
        conn.sendall(mesg)
        
      else:
        # Send work message (func_name,'split_file.mat','shared_file.mat')
        conn.sendall(mesg)

      # Wait for results (filename):
      if need_results:
        waitOnResults(conn, mpiComm, protocol)

    elif (worker_status.tag == KILL_TAG):
      if protocol == 'NETWORK':
        semaphore.acquire()
        memory.unlink()
        mapfile.close()

      conn.sendall("kill")
      conn.close()
      break

else:
  data = {'name': name, 'rank': rank}
  mpiComm.send(data, dest=0, tag=CHECKIN_TAG)
  worker_status = MPI.Status()

  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  sock.bind(('localhost', WORKER_PORT))
  sock.listen(1)

  parent_conn, child_conn = Pipe()
  p = Process(target=Rvis, args=(child_conn,))
  p.start()
  parent_conn.send([42, None, 'hello,vis'])

  cnn, addr = sock.accept()
  mesg=''
  mesg = cnn.recv(128)
  print 'got mesg:' + mesg
  while mesg != 'kill':
    mesg = mpiComm.recv(source=0, tag=MPI.ANY_TAG, status=worker_status)
    if (worker_status.tag == DO_WORK_TAG):
      print 'VIS got: ' + mesg + '\n'

      data = analyzeResults(mesg)

      cnn.sendall(data)
      mesg = cnn.recv(128)
      print 'got new mesg:' + mesg
    elif (worker_status.tag == KILL_TAG):
      print "VISKILL"
      kill = 1
      cnn.sendall("kill")
      cnn.close()
      break


# MPI : once rank 0 Matlab is dead, kill the workers
if(DEBUG):print 'kill is first ' + str(kill) + ' on rank ' + srank
#mpiComm.bcast(kill, root=0, tag=KILL_TAG)
if (rank==0):
  sock.close()
  kill = 1
  if interface == 'matlab':
    print 'You may now safely exit'
#if rank != 0:
#  s.close()
#  if(DEBUG):print 'killing ' + str(p.pid)
#  os.kill(p.pid, signal.SIGUSR1)
