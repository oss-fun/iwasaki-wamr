#!/bin/bash

cd ../build
#CURRENT=$(cd $(dirname $0);pwd)
#echo $CURRENT
make clean
cmake .. -DWAMR_BUILD_FAST_INTERP=0 -DWAMR_BUILD_SGX_IPFS=1 
make -j


cd ../enclave-sample
make clean
make SGX_MODE=HW
/opt/intel/sgxsdk/bin/x64/sgx_sign sign \
  -key private_key.pem \
  -enclave enclave.so \
  -out enclave.signed.so \
  -config Enclave/Enclave.config.xml
