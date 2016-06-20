#!/bin/bash

mkdir /usr/local/greenplum-db
export BLD_ARCH=rhel5_x86_64
cd gpdb_src
./configure --prefix=/usr/local/greenplum-db --enable-depend --enable-debug --with-python --with-libxml || exit 1
make || exit 1
make install || exit 1
cd ..
pushd /usr/local
tar -zcvf bin_gpdb_centos7.tar.gz greenplum-db
popd
mv /usr/local/bin_gpdb_centos7.tar.gz gpdb4_build/