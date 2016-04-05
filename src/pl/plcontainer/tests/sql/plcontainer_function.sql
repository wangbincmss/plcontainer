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

create or replace function vec8(arg1 _float8) returns _float8 as
$$
# container: plc_r
arg1
$$ language 'plcontainer';

create or replace function vec4(arg1 _float4) returns _float4 as
$$
# container: plc_r
arg1
$$ language 'plcontainer';

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

CREATE OR REPLACE FUNCTION pyintarr(arr int8[]) RETURNS int8 AS $$
# container: plc_python
def recsum(obj):
    s = 0
    if type(obj) is list:
        for x in obj:
            s += recsum(x)
    else:
        s = obj
    return s

return recsum(arr)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyintnulls(arr int8[]) RETURNS int8 AS $$
# container: plc_python
res = 0
for el in arr:
    if el is None:
        res += 1
return res
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyfloatarr(arr float8[]) RETURNS float8 AS $$
# container: plc_python
global num
num = 0

def recsum(obj):
    global num
    s = 0.0
    if type(obj) is list:
        for x in obj:
            s += recsum(x)
    else:
        num += 1
        s = obj
    return s

return recsum(arr)/float(num)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytextarr(arr varchar[]) RETURNS varchar AS $$
# container: plc_python
global res
res = ''

def recconcat(obj):
    global res
    if type(obj) is list:
        for x in obj:
            recconcat(x)
    else:
        res += '|' + obj
    return

recconcat(arr)
return res
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrint8(num int) RETURNS int8[] AS $BODY$
# container: plc_python
return [x for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrint8nulls() RETURNS int8[] AS $BODY$
# container: plc_python
return [1,2,3,None,5,6,None,8,9]
$BODY$ LANGUAGE plcontainer;

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

CREATE OR REPLACE FUNCTION py_plpy_get_record() RETURNS bool AS $$
# container: plc_python
q = """SELECT 't'::bool as a,
              1::smallint as b,
              2::int as c,
              3::bigint as d,
              4::float4 as e,
              5::float8 as f,
              'foobar'::varchar as g
    """
r = plpy.execute(q)
if len(q) != 1: return False
if q[0]['a'] != True: return False
if q[0]['b'] != 1: return False
if q[0]['c'] != 2: return False
if q[0]['d'] != 3: return False
if q[0]['e'] != 4.0: return False
if q[0]['f'] != 5.0: return False
if q[0]['g'] != 'foobar': return False
return True
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
