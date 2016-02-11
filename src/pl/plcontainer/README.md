## PL/Container

This is an implementation of trusted language execution engine capable of
bringing up Docker containers to isolate executors from the host OS, i.e.
implement sandboxing.

### Requirements

**note** this secure R and python proof of concept was only tested on CentOS
7.2, and you should use Vagrant to bring it up, see the details in
`vagrant/centos` directory of the current repository.

### Building

#### Building Greenplum

```shell
git clone https://github.com/greenplum-db/trusted-languages-poc.git
cd vagrant/centos
vagrant up
```

That's it, Greenplum is built and installed, database is up and running

### Building PL/Container Language

PL/Container is built by default together with Greenplum. In case you have made
changes to the code of PL/Container you can easily rebuild it:

1. Login to Vagrant: `vagrant ssh`
1. Go to the PL/Container directory: `cd /gpdb/src/pl/plcontainer`
1. Make it: `make install`

Database restart is not required to catch up new container execution library,
you can simply connect to the database and all the calls you will make to
plcontainer language would be held by new binaries you just built

### Building Containers

Containers are not built by default together with Greenplum, you have to build
them manually:

1. Login to Vagrant: `vagrant ssh`
1. Go to the PL/Container directory: `cd /gpdb/src/pl/plcontainer`
1. Make it: `make containers`

### Running the tests

1. Login to Vagrant: `vagrant ssh`
1. Go to the PL/Container test directory: `cd /gpdb/src/pl/plcontainer/tests`
1. Make it: `make`

### Desgin

The idea of this PoC is to use containers to run user defined PL. The
current implementation assume the pl function definition to have the
following structure:

```sql
CREATE FUNCTION stupidPython() RETURNS text AS $$
# container: plc_python
return 'hello from Python'
$$ LANGUAGE plcontainer;
```

There are a couple of things that are interesting here:

1. The function definition starts with the line `# container: plc_python` which
defines the name of the Docker image that will be used to start the container.
To check the list of available Docker images simply run `docker images`

1. The implementation assumes Docker container exposes some port, i.e. the
container is started by running `docker run -d -P <image>` to publish the
exposed port to a random port on the host. For an example of how to create a
compatible docker image see `Dockerfile.R` and `Dockerfile.python`

1. The `LANGUAGE` argument to Greenplum is `plcontainer`

1. At the moment two containers are implemented for PL/Container: R and Python.
For example, to define a function that uses a container that runs the `R`
interpreter, simply make the following definition:
```sql
CREATE FUNCTION stupidR() RETURNS text AS $$
# container: plc_r
return(log10(100))
$$ LANGUAGE plcontainer;
```
