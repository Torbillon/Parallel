#!/bin/bash
  
HDF5=/opt/hdf5/intel/mvapich2_ib
export CMAKE_PREFIX_PATH=$HDF5
cmake -DCMAKE_INSTALL_PREFIX:PATH=/home/tislam/Tools/perf-dump-perThread/BUILD-comet \
-DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/intel-14.0.2.cmake \
-DPAPI_PREFIX=/opt/papi/intel \
 ..

make VERBOSE=1                                                        
