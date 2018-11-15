#!/bin/sh
SRC="test_ocl.cpp ../RuntimeEngineOCL.cpp"
TARGET="test_ocl"
CXXFLAGS=" -DRUNTIME_OCL=1 -I/opt/rocm/opencl/include/ -O2 -std=c++11 -g"
CXXFLAGS="$CXXFLAGS -I../ -I../../ -I../../utils/feifei"
LDFLAGS="-L/opt/rocm/opencl/lib/x86_64 -lOpenCL"

rm -rf $TARGET
g++ $CXXFLAGS $SRC -o $TARGET $LDFLAGS
