#!/bin/bash

set -x

GPDATA=$1

# Configure profile
rm ~/.bashrc
printf '#!/bin/bash\n\n'                                     >> ~/.bashrc
printf '. /etc/bashrc\n\n'                                   >> ~/.bashrc
printf '\n# GPDB environment variables\nexport GPDB=/gpdb\n' >> ~/.bashrc
printf 'export GPHOME=/usr/local/greenplum-db\n'             >> ~/.bashrc
printf 'export GPDATA=/data\n'                               >> ~/.bashrc
printf 'export R_HOME=/usr/lib64/R\n'                        >> ~/.bashrc
printf 'if [ -e $GPHOME/greenplum_path.sh ]; then\n\t'       >> ~/.bashrc
printf 'source $GPHOME/greenplum_path.sh\nfi\n'              >> ~/.bashrc
source ~/.bashrc

mkdir $GPDATA/master
mkdir $GPDATA/primary

echo "127.0.0.1" > $GPDATA/hosts

GPCFG=$GPDATA/gpinitsystem_config
rm -f $GPCFG
printf "declare -a DATA_DIRECTORY=($GPDATA/primary $GPDATA/primary)\n" >> $GPCFG
printf "MASTER_HOSTNAME=127.0.0.1\n"                                   >> $GPCFG
printf "MACHINE_LIST_FILE=$GPDATA/hosts\n"                             >> $GPCFG
printf "MASTER_DIRECTORY=$GPDATA/master\n"                             >> $GPCFG
printf "ARRAY_NAME=\"GPDB\" \n"                                        >> $GPCFG
printf "SEG_PREFIX=gpseg\n"                                            >> $GPCFG
printf "PORT_BASE=40000\n"                                             >> $GPCFG
printf "MASTER_PORT=5432\n"                                            >> $GPCFG
printf "TRUSTED_SHELL=ssh\n"                                           >> $GPCFG
printf "CHECK_POINT_SEGMENTS=8\n"                                      >> $GPCFG
printf "ENCODING=UNICODE\n"                                            >> $GPCFG

$GPHOME/bin/gpinitsystem -a -c $GPDATA/gpinitsystem_config
printf '# Remember the master data directory location\n' >> ~/.bashrc
printf 'export MASTER_DATA_DIRECTORY=$GPDATA/master/gpseg-1\n' >> ~/.bashrc
source ~/.bashrc

gpstate || exit 1