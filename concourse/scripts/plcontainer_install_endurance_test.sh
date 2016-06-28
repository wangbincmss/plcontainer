#!/bin/bash

source ~/.bashrc

set -x

WORKDIR=$1
PLCGPPKG=$2
TMPDIR=$3
GPDBVER=$4

cd $WORKDIR

# Install PL/Container package
gppkg -i $PLCGPPKG/plcontainer-concourse.gppkg || exit 1
cp plcontainer_src/management/config/plcontainer_configuration-ci.xml $GPHOME/share/postgresql/plcontainer/plcontainer_configuration.xml
plcontainer-config --reset

rm -rf $TMPDIR
cp -r plcontainer_src $TMPDIR
cd $TMPDIR/tests

# Test version depends on GPDB release we are running on
if [ "$GPDBVER" == "gpdb5" ]; then
    python endurance_test.py -g 5 -t 1 -u gpadmin || exit 1
else
    python endurance_test.py -g 4 -t 1 -u gpadmin || exit 1
fi