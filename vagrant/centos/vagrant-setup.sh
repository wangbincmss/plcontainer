#!/bin/bash

set -x

# install packages needed to build and run GPDB
sudo yum -y groupinstall "Development tools"
sudo yum -y install ed
sudo yum -y install readline-devel
sudo yum -y install zlib-devel
sudo yum -y install curl-devel
sudo yum -y install bzip2-devel
sudo yum -y install python-devel
sudo yum -y install apr-devel
sudo yum -y install libevent-devel
sudo yum -y install openssl-libs openssl-devel
sudo yum -y install libyaml libyaml-devel
sudo yum -y install epel-release
sudo yum -y install htop
sudo yum -y install perl-Env
sudo yum -y install R R-devel R-core
sudo yum -y install libxml2 libxml2-devel
wget https://bootstrap.pypa.io/get-pip.py
sudo python get-pip.py
sudo pip install psi lockfile paramiko setuptools epydoc psutil
rm get-pip.py

# Install Docker
sudo cp /vagrant/docker.repo /etc/yum.repos.d/
sudo yum -y install docker-engine-1.11.0
sudo systemctl start docker.service
sudo systemctl enable docker.service

# Add vagrant to Docker group
sudo usermod -aG docker vagrant

# Misc
sudo yum -y install vim mc psmisc
