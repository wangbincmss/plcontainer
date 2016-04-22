FROM centos:7.2.1511

# Installing R module
RUN yum -y install epel-release
RUN yum -y install R R-devel R-core

# Running R client inside of container
ADD ./src/rclient /clientdir
RUN cp /clientdir/librcall.so /usr/lib64
ENV R_HOME /usr/lib64/R
EXPOSE 8080
WORKDIR /clientdir
CMD ["./client"]
