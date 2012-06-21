mkdir build
cd build
wget https://mpi4py.googlecode.com/files/mpi4py-1.3.tar.gz
tar xzvf mpi4py-1.3.tar.gz
cd mpi4py-1.3
#module purge
#module load python/2.7.2 numpy/1.6.1 mpi-tor/openmpi-1.4.5_gcc-4.4.6
TODO: add aciss configure profile
python setup.py build --mpi=aciss
#python setup.py install --user

echo 'add this to your path:'
#export PATH=$PATH:~/MATLAByrinth-install/MATLABYrinth/bin
echo 'Also set the MATLABYRINT environment variable:'

#export MATLABYRINTH='~/MATLAByrinth-install/MATLAByrinth'

echo 'If it does not exist, create the following directory and file:'
#    ~/Documents/MATLAB/startup.m

echo 'and append this line to the startup.m file:'
#    addpath ~/MATLAByrinth-install/MATLABYrinth/src/matlab

#Refer to the MATLAByrinth README file to see how to test the installation.

