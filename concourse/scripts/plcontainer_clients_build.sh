#!/bin/bash

set -x

# Clean up and creating target directories
rm -rf plcontainer_clients_build/python26 && mkdir plcontainer_clients_build/python26
rm -rf plcontainer_clients_build/python27 && mkdir plcontainer_clients_build/python27
rm -rf plcontainer_clients_build/r31 && mkdir plcontainer_clients_build/r31
rm -rf plcontainer_clients_build/r32 && mkdir plcontainer_clients_build/r32

# Build Python 2.7 and R 3.2 - default ones
cd plcontainer_src
export R_HOME=/usr/lib64/R
pushd src/pyclient
make clean && make
popd
pushd src/rclient
make clean && make
popd
cp src/pyclient/client     ../plcontainer_clients_build/python27/client
cp src/rclient/client      ../plcontainer_clients_build/r32/client
cp src/rclient/librcall.so ../plcontainer_clients_build/r32/librcall.so
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
cp src/pyclient/client     ../plcontainer_clients_build/python26/client
cp src/rclient/client      ../plcontainer_clients_build/r31/client
cp src/rclient/librcall.so ../plcontainer_clients_build/r31/librcall.so

# Build Python 3.4 and Python 3.5
export PYTHON=python3
pushd src/pyclient
make clean && make
popd
cp src/pyclient/client    ../plcontainer_clients_build/python34/client
unset PYTHON
export LD_LIBRARY_PATH=/usr/local/anaconda/lib
export PYTHONHOME=/usr/local/anaconda
export PATH=/usr/local/anaconda/bin:$PATH
pushd src/pyclient
make clean && make
popd
cp src/pyclient/client    ../plcontainer_clients_build/python35/client
