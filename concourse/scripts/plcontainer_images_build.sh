#!/bin/bash

set -x

# Put GPDB binaries in place to get pg_config
cp bin_gpdb_centos7/bin_gpdb_centos7.tar.gz /usr/local
pushd /usr/local
tar zxvf bin_gpdb_centos7.tar.gz
popd
source /usr/local/greenplum-db/greenplum_path.sh || exit 1

# Preparing for Docker and starting it
source plcontainer_src/concourse/scripts/docker_scripts.sh
start_docker || exit 1
echo 'agrishchenko@pivotal.io\n\n' | docker login -u agrishchenko -p MyDockerPassword9283 || exit 1

# Making so would change container tags to "devel"
export CIBUILD=1

# Building Python 2.6 and R 3.1
cp bin_python26_client/client      plcontainer_src/src/pyclient/bin/
cp bin_r31_client/client           plcontainer_src/src/rclient/bin/
cp bin_r31_clientlib/librcall.so   plcontainer_src/src/rclient/bin/

pushd plcontainer_src
make container_python_shared || exit 1
make container_r_shared      || exit 1
popd

# Building Python 2.7, R 3.2 and Anaconda
cp bin_python27_client/client      plcontainer_src/src/pyclient/bin/
cp bin_r32_client/client           plcontainer_src/src/rclient/bin/
cp bin_r32_clientlib/librcall.so   plcontainer_src/src/rclient/bin/

pushd plcontainer_src
make container_python   || exit 1
make container_r        || exit 1
make container_anaconda || exit 1
popd

# Publising new images to Docker Hub
docker push pivotaldata/plcontainer_python:devel || exit 1
docker push pivotaldata/plcontainer_r:devel || exit 1
docker push pivotaldata/plcontainer_r_shared:devel || exit 1
docker push pivotaldata/plcontainer_python_shared:devel || exit 1
docker push pivotaldata/plcontainer_anaconda:devel || exit 1

stop_docker || exit 1

echo 'OK' > plcontainer_images_build/status.log