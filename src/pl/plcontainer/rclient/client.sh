#!/bin/bash

if [ -d /usr/local/greenplum-db ] ; then
    source /usr/local/greenplum-db/greenplum_path.sh
else
    export R_HOME=/usr/lib64/R
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/clientdir

cd /clientdir

./client

