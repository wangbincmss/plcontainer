#!/bin/bash

if [ -d /usr/local/greenplum-db ] ; then
    source /usr/local/greenplum-db/greenplum_path.sh
fi

cd /clientdir

./client