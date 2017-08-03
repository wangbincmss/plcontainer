#!/bin/bash

set -x

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
