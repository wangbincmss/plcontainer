#!/bin/bash

set -ex

docker_build() {
	local node=$1
	scp -r bin_python26_client node:~/
	scp -r bin_python27_client node:~/
	scp -r plcontainer_gpdb_build $node:~/
	scp -r plcontainer_src $node:~/

	ssh $node "bash -c \" \
	cp bin_python27_client/client plcontainer_src/src/pyclient/bin/; \
	chmod 777 plcontainer_src/src/pyclient/bin/client; \
	pushd plcontainer_src; \
	docker build -f dockerfiles/Dockerfile.python.shared.centos6 -t pivotaldata/plcontainer_python_shared:devel ./ ; \
	popd; \
	\""
}

docker_build sdw1
docker_build mdw

