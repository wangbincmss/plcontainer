/* ========================== R Functions ========================== */

CREATE OR REPLACE FUNCTION rlog100() RETURNS text AS $$
# container: plc_r
return(log10(100))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rbool(b bool) RETURNS bool AS $$
# container: plc_r
return (as.logical(b))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rint(i int2) RETURNS int2 AS $$
# container: plc_r
return (as.integer(i)+1)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rint(i int4) RETURNS int4 AS $$
# container: plc_r
return (as.integer(i)+2)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rint(i int8) RETURNS int8 AS $$
# container: plc_r
return (as.integer(i)+3)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rfloat(f float4) RETURNS float4 AS $$
# container: plc_r
return (as.numeric(f)+1)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rfloat(f float8) RETURNS float8 AS $$
# container: plc_r
return (as.numeric(f)+2)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rtext(arg varchar) RETURNS varchar AS $$
# container: plc_r
return(paste(arg,'foo',sep=''))
$$ LANGUAGE plcontainer;

create or replace function rtest_mia() returns int[] as $$
#container:plc_r
as.matrix(array(1:10,c(2,5)))
$$ language plcontainer;

CREATE OR REPLACE FUNCTION rlog100_shared() RETURNS text AS $$
# container: plc_r_shared
return(log10(100))
$$ LANGUAGE plcontainer;

create or replace function rpg_spi_exec(arg1 text) returns text as $$
#container: plc_r
(pg.spi.exec(arg1))[[1]]
$$ language plcontainer;

-- create type for next function
create type user_type as (fname text, lname text, username text);

create or replace function rtest_spi_tup(arg1 text) returns setof user_type as $$
#container: plc_r
pg.spi.exec(arg1)
$$ language plcontainer;

create or replace function rtest_spi_ta(arg1 text) returns setof record as $$
#container: plc_r
pg.spi.exec(arg1)
$$ language plcontainer;

/* ========================== Python Functions ========================== */

CREATE OR REPLACE FUNCTION pylog100() RETURNS double precision AS $$
# container: plc_python
import math
return math.log10(100)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylog(a integer, b integer) RETURNS double precision AS $$
# container: plc_python
import math
return math.log(a, b)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybool(b bool) RETURNS bool AS $$
# container: plc_python
return b
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyint(i int2) RETURNS int2 AS $$
# container: plc_python
return i+1
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyint(i int4) RETURNS int4 AS $$
# container: plc_python
return i+2
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyint(i int8) RETURNS int8 AS $$
# container: plc_python
return i+3
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyfloat(f float4) RETURNS float4 AS $$
# container: plc_python
return f+1
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyfloat(f float8) RETURNS float8 AS $$
# container: plc_python
return f+2
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytext(t text) RETURNS text AS $$
# container: plc_python
return t+'bar'
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pywriteFile() RETURNS text AS $$
# container: plc_python
f = open("/tmp/foo", "w")
f.write("foobar")
f.close
return 'wrote foobar to /tmp/foo'
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyconcat(a text, b text) RETURNS text AS $$
# container: plc_python
import math
return a + b
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyconcatall() RETURNS text AS $$
# container: plc_python
res = plpy.execute('select fname from users order by 1')
names = map(lambda x: x['fname'], res)
return reduce(lambda x,y: x + ',' + y, names)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pynested_call_one(a text) RETURNS text AS $$
# container: plc_python
q = "SELECT pynested_call_two('%s')" % a
r = plpy.execute(q)
return r[0]
$$ LANGUAGE plcontainer ;

CREATE OR REPLACE FUNCTION pynested_call_two(a text) RETURNS text AS $$
# container: plc_python
q = "SELECT pynested_call_three('%s')" % a
r = plpy.execute(q)
return r[0]
$$ LANGUAGE plcontainer ;

CREATE OR REPLACE FUNCTION pynested_call_three(a text) RETURNS text AS $$
# container: plc_python
return a
$$ LANGUAGE plcontainer ;

CREATE OR REPLACE FUNCTION pyinvalid_function() RETURNS double precision AS $$
# container: plc_python
import math
return math.foobar(a, b)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyinvalid_syntax() RETURNS double precision AS $$
# container: plc_python
import math
return math.foobar(a,
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylog100_shared() RETURNS double precision AS $$
# container: plc_python_shared
import math
return math.log10(100)
$$ LANGUAGE plcontainer;
