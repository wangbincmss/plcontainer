#!/bin/bash

set -x

cp $1/$1.tar.gz /usr/local
pushd /usr/local
tar zxvf $1.tar.gz
popd
source /usr/local/greenplum-db/greenplum_path.sh

pushd plcontainer_src
make clean
pushd package
make cleanall && make
popd
popd
cp plcontainer_src/package/plcontainer-*.gppkg $2/