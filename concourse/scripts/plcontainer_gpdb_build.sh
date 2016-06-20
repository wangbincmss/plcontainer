#!/bin/bash

cp bin_gpdb_centos7/bin_gpdb_centos7.tar.gz /usr/local
pushd /usr/local
tar zxvf bin_gpdb_centos7.tar.gz
popd
source /usr/local/greenplum-db/greenplum_path.sh

pushd plcontainer_src
make clean
pushd package
make cleanall && make
popd
popd
cp plcontainer_src/package/plcontainer-*.gppkg plcontainer_gpdb_build/