#!/bin/bash

set -e -x

ssh mdw "bash -c \" \
export MASTER_DATA_DIRECTORY=/data/gpdata/master/gpseg-1; \
source /usr/local/greenplum-db-devel/greenplum_path.sh; \
gppkg -i plcontainer_gpdb_build/plcontainer-concourse-centos6.gppkg; \
plcontainer-config --reset; \
gpstop -arf; \
psql -d postgres -f /usr/local/greenplum-db-devel/share/postgresql/plcontainer/plcontainer_install.sql; \
psql -d postgres -f plcontainer_src/concourse/scripts/function_setup.sql; \
psql -d postgres -f plcontainer_src/concourse/scripts/perf_prepare1.sql; \
nohup psql -d postgres -f plcontainer_src/concourse/scripts/perf_pl1.sql > perf_pl1 2>&1; \
nohup psql -d postgres -f plcontainer_src/concourse/scripts/perf_py1.sql > perf_py1 2>&1; \
\""

scp mdw:~/perf_pl1 plcontainer_perf_result/perf_pl1
scp mdw:~/perf_py1 plcontainer_perf_result/perf_py1
