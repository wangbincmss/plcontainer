#!/bin/bash

set -e -x

export MASTER_DATA_DIRECTORY=/data/gpdata/master/gpseg-1
source /usr/local/greenplum-db-devel/greenplum_path.sh
gppkg -i plcontainer_gpdb_build/plcontainer-concourse-centos6.gppkg
plcontainer-config --reset
gpstop -arf
psql -d f1 -f /usr/local/greenplum-db-devel/share/postgresql/plcontainer/plcontainer_install.sql
psql -d f1 -f plcontainer_src/concourse/scripts/f1.sql
