#!/bin/bash
set -ex

ssh centos@mdw "sudo bash -c \"sudo rpm -iUvh http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm; \
 sudo yum update -y; \
 sudo yum -y install docker-io; \
 sudo service docker start; \
 sudo groupadd docker; \
 sudo chown root:docker /var/run/docker.sock; \
 sudo usermod -a -G docker gpadmin; \
\""

ssh centos@sdw1 "sudo bash -c \"sudo rpm -iUvh http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm; \
 sudo yum update -y; \
 sudo yum -y install docker-io; \
 sudo service docker start; \
 sudo groupadd docker; \
 sudo chown root:docker /var/run/docker.sock; \
 sudo usermod -a -G docker gpadmin; \
\""
