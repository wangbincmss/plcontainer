#!/bin/bash

set -x

# Build Python 2.7 and R 3.2 - default ones
cd plcontainer_src
export R_HOME=/usr/lib64/R
pushd src/pyclient
make clean && make
popd
pushd src/rclient
make clean && make
popd
cp src/pyclient/client    ../plcontainer_clients_build/python27/client
cp src/rclient/client     ../plcontainer_clients_build/r32/client
cp src/rclient/libcall.so ../plcontainer_clients_build/r32/libcall.so
unset R_HOME

# Build Python 2.6 and R 3.1 - the ones shipped with GPDB
cp bin_gpdb4off_centos7/bin_gpdb4off_centos7.tar.gz /usr/local
pushd /usr/local
tar zxvf $1.tar.gz
popd
source /usr/local/greenplum-db/greenplum_path.sh
pushd src/pyclient
make clean && make
popd
pushd src/rclient
make clean && make
popd
cp src/pyclient/client    ../plcontainer_clients_build/python26/client
cp src/rclient/client     ../plcontainer_clients_build/r31/client
cp src/rclient/libcall.so ../plcontainer_clients_build/r31/libcall.so