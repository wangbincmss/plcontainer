#!/bin/bash -l

set -eox pipefail

ccp_src/aws/setup_ssh_to_cluster.sh
plcontainer_src/concourse/script/docker_install.sh
plcontainer_src/concourse/script/docker_build.sh
plcontainer_src/concourse/script/run_plcontainer.sh


