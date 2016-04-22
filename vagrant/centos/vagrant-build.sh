#!/bin/bash

# Prepare target directory for GPDB installation
sudo rm -rf /usr/local/greenplum-db
sudo mkdir /usr/local/greenplum-db
sudo chown -R vagrant:vagrant /usr/local/greenplum-db

# Get the GPDB from github
sudo rm -rf /gpdb
sudo mkdir /gpdb
sudo chown -R vagrant:vagrant /gpdb
git clone https://github.com/greenplum-db/gpdb.git /gpdb

# Build GPDB
cd /gpdb
./configure --prefix=/usr/local/greenplum-db --enable-depend --enable-debug --with-python --with-libxml || exit 1
sudo make || exit 1
sudo make install || exit 1
sudo chown -R vagrant:vagrant /usr/local/greenplum-db

