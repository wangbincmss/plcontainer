#!/bin/bash

set -e -x

export MASTER_DATA_DIRECTORY=/data/gpdata/master/gpseg-1
gppkg -i plcontainer_gpdb_build/plcontainer-concourse-centos6.gppkg
source /usr/local/greenplum-db-devel/greenplum_path.sh
plcontainer-config --reset
gpstop -arf
psql -d f1 -f /usr/local/greenplum-db-devel/share/postgresql/plcontainer/plcontainer_install.sql

