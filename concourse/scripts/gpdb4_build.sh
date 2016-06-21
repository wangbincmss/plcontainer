#!/bin/bash

set -x

mkdir /usr/local/greenplum-db
export BLD_ARCH=rhel5_x86_64
cd gpdb4_src
CFLAGS="-Wno-error=unused-but-set-variable" ./configure --prefix=/usr/local/greenplum-db --enable-depend --enable-debug --with-python --with-libxml || exit 1
make prefix=/usr/local/greenplum-db || exit 1
make install prefix=/usr/local/greenplum-db || exit 1
cd ..
pushd /usr/local
tar -zcvf bin_gpdb4git_centos7.tar.gz greenplum-db
popd
mv /usr/local/bin_gpdb4git_centos7.tar.gz gpdb4_build/