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
echo 'gpdb-trusted-languages@pivotal.io\n\n' | docker login -u gpdbplcontainer -p erDl7rLqb738 || exit 1

# Making so would change container tags to "devel"
export CIBUILD=1

# Building Python 2.6 and R 3.1
cp bin_python26_client/client      plcontainer_src/src/pyclient/bin/
cp bin_r31_client/client           plcontainer_src/src/rclient/bin/
cp bin_r31_clientlib/librcall.so   plcontainer_src/src/rclient/bin/
chmod 777 plcontainer_src/src/pyclient/bin/client
chmod 777 plcontainer_src/src/rclient/bin/client
chmod 777 plcontainer_src/src/rclient/bin/librcall.so

pushd plcontainer_src
make container_python_shared || exit 1
make container_r_shared      || exit 1
popd

# Building Python 2.7, R 3.2 and Anaconda
cp bin_python27_client/client      plcontainer_src/src/pyclient/bin/
cp bin_r32_client/client           plcontainer_src/src/rclient/bin/
cp bin_r32_clientlib/librcall.so   plcontainer_src/src/rclient/bin/
chmod 777 plcontainer_src/src/pyclient/bin/client
chmod 777 plcontainer_src/src/rclient/bin/client
chmod 777 plcontainer_src/src/rclient/bin/librcall.so

pushd plcontainer_src
make container_python    || exit 1
make container_r         || exit 1
make container_anaconda  || exit 1
popd

# Building Python 3.4
cp bin_python34_client/client      plcontainer_src/src/pyclient/bin/
pushd plcontainer_src
make container_python3   || exit 1
popd

# Building Anaconda3
cp bin_python35_client/client      plcontainer_src/src/pyclient/bin/
pushd plcontainer_src
make container_anaconda3 || exit 1
popd

# Publising new images to Docker Hub
docker push pivotaldata/plcontainer_python:devel || exit 1
docker push pivotaldata/plcontainer_python3:devel || exit 1
docker push pivotaldata/plcontainer_r:devel || exit 1
docker push pivotaldata/plcontainer_r_shared:devel || exit 1
docker push pivotaldata/plcontainer_python_shared:devel || exit 1
docker push pivotaldata/plcontainer_anaconda:devel || exit 1
docker push pivotaldata/plcontainer_anaconda3:devel || exit 1

docker save pivotaldata/plcontainer_python:devel pivotaldata/plcontainer_python3:devel \
        pivotaldata/plcontainer_r:devel pivotaldata/plcontainer_r_shared:devel \
        pivotaldata/plcontainer_python_shared:devel pivotaldata/plcontainer_anaconda:devel \
        pivotaldata/plcontainer_anaconda3:devel | gzip -c > plcontainer_images_build/plcontainer-devel-images.tar.gz

stop_docker || exit 1