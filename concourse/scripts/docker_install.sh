#!/bin/bash
set -ex

install_docker() {
	local node=$1
	ssh centos@$node "sudo bash -c \"sudo rpm -iUvh http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm; \
 	sudo yum update -y; \
 	sudo yum -y install docker-io; \
 	sudo service docker start; \
 	sudo groupadd docker; \
 	sudo chown root:docker /var/run/docker.sock; \
 	sudo usermod -a -G docker gpadmin; \
	\""
}

install_docker mdw
install_docker sdw1

