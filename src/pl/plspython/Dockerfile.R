FROM centos:7

# currently the images are 800M for the python container and 1G for
# the R container. If you want to create a smaller image, you can
# do the following:
#
# 1. move the ADD command before the first RUN.
#
# 2. combine all RUN commands into one (i.e. using && )
#
# 3. append to the RUN command (i.e. && ...) commands to `rm -rf
# /postgres` and uninstall uneeded packages (e.g. apt-get remove
# build-essential pkg-config ...)

RUN yum -y update
RUN yum -y groupinstall "Development Tools"
RUN yum -y install pkgconfig
RUN yum -y install epel-release
RUN yum -y install R R-devel R-core

ADD . /postgres/

#./configure --without-readline --without-zlib && \

RUN cd /postgres && \
        cd src/pl/plspython/rclient && \
        make clean && \
        make && \
        cp client /

ENV R_HOME /usr/lib64/R

EXPOSE 8080

CMD ["/client"]
