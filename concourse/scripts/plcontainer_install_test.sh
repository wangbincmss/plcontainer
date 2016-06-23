#!/bin/bash

source ~/.bashrc

set -x

WORKDIR=$1
PLCGPPKG=$2

cd $WORKDIR

# Install PL/Container package
gppkg -i $PLCGPPKG/plcontainer-concourse.gppkg || exit 1
cp plcontainer_src/management/config/plcontainer_configuration-ci.xml $GPHOME/share/postgresql/plcontainer/plcontainer_configuration.xml
plcontainer-config --reset

rm -rf /tmp/localplccopy
cp -r plcontainer_src /tmp/localplccopy
cd /tmp/localplccopy/tests
make tests