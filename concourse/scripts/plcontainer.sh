#!/bin/bash -l

set -eox pipefail

ccp_src/aws/setup_ssh_to_cluster.sh
plcontainer_src/concourse/scripts/docker_install.sh
plcontainer_src/concourse/scripts/docker_build.sh
plcontainer_src/concourse/scripts/run_plcontainer.sh

exit 1
