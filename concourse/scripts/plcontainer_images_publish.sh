#!/bin/bash

set -x

cp bin_gpdb_centos7/bin_gpdb_centos7.tar.gz /usr/local
pushd /usr/local
tar zxvf bin_gpdb_centos7.tar.gz
popd
source /usr/local/greenplum-db/greenplum_path.sh

# Preparing for Docker and starting it
source plcontainer_src/concourse/scripts/docker_scripts.sh
start_docker || exit 1

# Loading images
docker load -i plcontainer_devel_images/plcontainer-devel-images.tar.gz

pushd plcontainer_src

echo 'gpdb-trusted-languages@pivotal.io\n\n' | docker login -u gpdbplcontainer -p erDl7rLqb738 || exit 1
RETCODE=$?
if [ $RETCODE -ne 0 ]; then
    echo "PL/Container Docker Hub login failed"
    stop_docker
    exit $RETCODE
fi

make pushdevel 
RETCODE=$?
if [ $RETCODE -ne 0 ]; then
    echo "PL/Container 'make pushdevel' failed"
    stop_docker
    exit $RETCODE
fi

stop_docker