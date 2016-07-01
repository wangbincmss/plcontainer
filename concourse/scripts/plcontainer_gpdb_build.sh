#!/bin/bash

set -x

GPDBBIN=$1
OUTPUT=$2
MODE=$3

cp $GPDBBIN/$GPDBBIN.tar.gz /usr/local
pushd /usr/local
tar zxvf $GPDBBIN.tar.gz
popd
source /usr/local/greenplum-db/greenplum_path.sh

if [ "$MODE" == "test" ]; then
    export CIBUILD=1
fi

pushd plcontainer_src
make clean
pushd package
make cleanall && make
popd
popd

if [ "$MODE" == "test" ]; then
    cp plcontainer_src/package/plcontainer-*.gppkg $OUTPUT/plcontainer-concourse.gppkg
else
    cp plcontainer_src/package/plcontainer-*.gppkg $OUTPUT/
fi