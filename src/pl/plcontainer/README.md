## PL/Container

This is a secure python pl implementation that uses containers to
isolate python code in its own sandbox.

### Requirements

**note** this secure R and python proof of concept was only tested on
Linux. You should use Vagrant to bring it up, see the `vagrant/centos` directory

### Building

#### Building postgres

1. `git clone https://github.com/greenplum-db/trusted-languages-poc.git`
1. `cd vagrant/centos`
1. `vagrant up`

### Starting the server

1. Login to Vagrant: `vagrant ssh`
1. Go under gpadmin user: `sudo su - gpadmin`
1. Start the Greenplum: `gpstart -a`

### Running the tests

1. `cd src/pl/plcontainer/tests`
1. `sudo make`

### Desgin

The idea of this poc is to use containers to run user defined PL. The
current implementation assume the pl function definition to have the
following structure:

```sql
CREATE FUNCTION stupidPython() RETURNS text AS $$
# container: plc_python
return 'hello from Python'
$$ LANGUAGE plcontainer;
```

There are a couple of things that are interesting here:

1. The function definition starts with the line `# container:
plc_python` which defines the name of the docker image name that will
be used to start the container.

1. The implementation assumes the docker container exposes some port,
i.e. the container is started by running `docker run -d -P <image>` to
publish the exposed port to a random port on the host. For an example
of how to create a compatible docker image see `tests/Dockerfile.R`
and `tests/Dockerfile.python`

1. The `LANGUAGE` argument to postgres is `plcontainer`. This name is
misleading since the same language can be used to define a function
that uses a container that runs the `R` interpretter, e.g.:
```sql
CREATE FUNCTION stupidR() RETURNS text AS $$
# container: plc_r
return(log10(100))
$$ LANGUAGE plcontainer;
```
This is just a left over from the experiment since it started as a
secure python PL. **note** this can be easily changed to be plc
(i.e. PL in containers).
