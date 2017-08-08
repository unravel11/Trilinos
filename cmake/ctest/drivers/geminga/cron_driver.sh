#!/bin/bash

echo
echo "Starting nightly Trilinos development testing on geminga: `date`"
echo

#
# TrilinosDriver settings:
#

export TDD_PARALLEL_LEVEL=2

# Trilinos settings:
#

# Submission mode for the *TrilinosDriver* dashboard
export TDD_CTEST_TEST_TYPE=Nightly

export TDD_DEBUG_VERBOSE=1
export TDD_FORCE_CMAKE_INSTALL=0
export TRIBITS_TDD_USE_SYSTEM_CTEST=1

#export CTEST_DO_SUBMIT=FALSE
#export CTEST_START_WITH_EMPTY_BINARY_DIRECTORY=FALSE

# Machine specific environment
#
export TDD_HTTP_PROXY="http://sonproxy.sandia.gov:80"
export TDD_HTTPS_PROXY="https://sonproxy.sandia.gov:80"
export http_proxy="http://sonproxy.sandia.gov:80"
export https_proxy="https://sonproxy.sandia.gov:80"

. ~/.bashrc

# If you update the list of modules, go to ~/code/trilinos-test/trilinos/ and
# do "git pull". Otherwise, the tests could fail on the first night, as we
# would first run old cron_driver.sh and only then pull

# ===========================================================================
export CTEST_CONFIGURATION="default"
module load openmpi/1.10.0
module load gcc/5.2.0
module load valgrind/3.10.1

# Remove colors (-fdiagnostics-color) from OMPI flags
# It may result in non-XML characters on the Dashboard
export OMPI_CFLAGS=`echo $OMPI_CFLAGS | sed 's/-fdiagnostics-color//'`
export OMPI_CXXFLAGS=`echo $OMPI_CXXFLAGS | sed 's/-fdiagnostics-color//'`

echo "Configuration = $CTEST_CONFIGURATION"
env

export OMP_NUM_THREADS=2

# Machine independent cron_driver:
SCRIPT_DIR=`cd "\`dirname \"$0\"\`";pwd`
$SCRIPT_DIR/../cron_driver.py

module unload valgrind
module unload gcc
module unload openmpi
# ===========================================================================
export CTEST_CONFIGURATION="broken"
module load openmpi/1.10.0
module load gcc/5.2.0
module load valgrind/3.10.1

# Remove colors (-fdiagnostics-color) from OMPI flags
# It may result in non-XML characters on the Dashboard
export OMPI_CFLAGS=`echo $OMPI_CFLAGS | sed 's/-fdiagnostics-color//'`
export OMPI_CXXFLAGS=`echo $OMPI_CXXFLAGS | sed 's/-fdiagnostics-color//'`

echo "Configuration = $CTEST_CONFIGURATION"
env

export OMP_NUM_THREADS=2

# Machine independent cron_driver:
SCRIPT_DIR=`cd "\`dirname \"$0\"\`";pwd`
$SCRIPT_DIR/../cron_driver.py

module unload valgrind
module unload gcc
module unload openmpi
# ===========================================================================
export CTEST_CONFIGURATION="nvcc_wrapper"
#module load openmpi/1.10.0
#module load gcc/4.9.2
#module load cuda/7.5-gcc
#module load nvcc-wrapper/gcc

module load sems-env
module load kokkos-env
module load sems-cmake/3.5.2
module load sems-gcc/5.3.0
module load sems-boost/1.58.0/base
module load sems-python/2.7.9
module load sems-zlib/1.2.8/base
module load kokkos-openmpi/1.8.7/cuda
module load kokkos-cuda/8.0.44
module load kokkos-nvcc_wrapper/1
module load sems-hdf5/1.8.12/parallel
module load sems-netcdf/4.3.2/base

# Remove colors (-fdiagnostics-color) from OMPI flags
# It may result in non-XML characters on the Dashboard
export OMPI_CFLAGS=`echo $OMPI_CFLAGS | sed 's/-fdiagnostics-color//'`

echo "OMPI_CXXFLAGS before $OMPI_CXXFLAGS"
export OMPI_CXXFLAGS=`echo $OMPI_CXXFLAGS | sed 's/-fdiagnostics-color//'`
echo "OMPI_CXXFLAGS after $OMPI_CXXFLAGS"

echo "Configuration = $CTEST_CONFIGURATION"
env

export CUDA_LAUNCH_BLOCKING=1

# Machine independent cron_driver:
SCRIPT_DIR=`cd "\`dirname \"$0\"\`";pwd`
$SCRIPT_DIR/../cron_driver.py

#module unload nvcc-wrapper
#module unload cuda
#module unload gcc
#module unload openmpi

module unload sems-env
module unload kokkos-env
module unload sems-cmake/3.5.2
module unload sems-gcc/5.3.0
module unload sems-boost/1.58.0/base
module unload sems-python/2.7.9
module unload sems-zlib/1.2.8/base
module unload kokkos-openmpi/1.8.7/cuda
module unload kokkos-cuda/8.0.44
module unload kokkos-nvcc_wrapper/1
module unload sems-hdf5/1.8.12/parallel
module unload sems-netcdf/4.3.2/base
# ===========================================================================

echo
echo "Ending nightly Trilinos development testing on geminga: `date`"
echo
