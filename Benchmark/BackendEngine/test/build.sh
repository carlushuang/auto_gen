#!/bin/sh
SRC="test_ocl.cpp ../BackendEngineOCL.cpp ../BackendEngineHSA.cpp"
TARGET="test_ocl"
CXXFLAGS="-O2 -std=c++11 -g"
CXXFLAGS="$CXXFLAGS -DRUNTIME_OCL=1 -I/opt/rocm/opencl/include/ "
CXXFLAGS="$CXXFLAGS -DRUNTIME_HSA=1 -I/opt/rocm/hsa/include/hsa/ "
CXXFLAGS="$CXXFLAGS -I../ -I../../ -I../../utils/feifei"
LDFLAGS="-L/opt/rocm/opencl/lib/x86_64 -lOpenCL"
LDFLAGS="$LDFLAGS -L/opt/rocm/lib/ -lhsa-runtime64"


rm -rf $TARGET
g++ $CXXFLAGS $SRC -o $TARGET $LDFLAGS
