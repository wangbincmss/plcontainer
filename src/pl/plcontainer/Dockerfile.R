FROM centos:7

RUN yum -y install epel-release
RUN yum -y install R R-devel R-core

ADD ./rclient /clientdir

ENV R_HOME /usr/lib64/R

EXPOSE 8080

CMD ["/clientdir/client"]
