FROM pivotaldata/plcontainer_r_base:0.1

# Running R client inside of container
ADD ./src/rclient/bin /clientdir
RUN cp /clientdir/librcall.so /usr/lib64
ENV R_HOME /usr/lib64/R
EXPOSE 8080
WORKDIR /clientdir