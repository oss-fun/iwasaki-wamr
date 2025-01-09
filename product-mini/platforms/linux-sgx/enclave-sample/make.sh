#!/bin/bash
make clean
make SGX_MODE=HW
/opt/intel/sgxsdk/bin/x64/sgx_sign sign \
  -key private_key.pem \
  -enclave enclave.so \
  -out enclave.signed.so \
  -config Enclave/Enclave.config.xml
