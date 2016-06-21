#!/bin/bash

set -x

source plcontainer_src/concourse/scripts/docker_scripts.sh
start_docker
docker version
stop_docker

echo 'OK' > plcontainer_images_build/status.log