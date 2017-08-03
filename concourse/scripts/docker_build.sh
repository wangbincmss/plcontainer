#!/bin/bash

set -x

scp -r bin_python26_client mdw:~/
scp -r bin_python27_client mdw:~/
scp -r plcontainer_gpdb_build mdw:~/
scp -r plcontainer_src mdw:~/

ssh sdw1 "bash -c \" \
cp bin_python27_client/client plcontainer_src/src/pyclient/bin/; \
chmod 777 plcontainer_src/src/pyclient/bin/client; \
pushd plcontainer_src; \
docker build -f dockerfiles/Dockerfile.python.shared.centos6 -t pivotaldata/plcontainer_python_shared:devel ./ ; \
popd; \
\""

ssh mdw "bash -c \" \
cp bin_python27_client/client plcontainer_src/src/pyclient/bin/; \
chmod 777 plcontainer_src/src/pyclient/bin/client; \
pushd plcontainer_src; \
docker build -f dockerfiles/Dockerfile.python.shared.centos6 -t pivotaldata/plcontainer_python_shared:devel ./ ; \
popd; \
\""
