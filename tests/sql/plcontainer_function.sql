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

CREATE OR REPLACE FUNCTION rnumeric(n numeric) RETURNS numeric AS $$
# container: plc_r
return (n+3.0)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rtimestamp(t timestamp) RETURNS timestamp AS $$
# container: plc_r
options(digits.secs = 6)
tmp <- strptime(t,'%Y-%m-%d %H:%M:%OS')
return (as.character(tmp + 3600))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rtimestamptz(t timestamptz) RETURNS timestamptz AS $$
# container: plc_r
t
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rtext(arg varchar) RETURNS varchar AS $$
# container: plc_r
return(paste(arg,'foo',sep=''))
$$ LANGUAGE plcontainer;

create or replace function rtest_mia() returns int[] as $$
#container:plc_r
as.matrix(array(1:10,c(2,5)))
$$ language plcontainer;

create or replace function vec(arg1 _float8) returns _float8 as
$$
# container: plc_r
arg1+1
$$ language 'plcontainer';

create or replace function vec(arg1 _float4) returns _float4 as
$$
# container: plc_r
arg1+1
$$ language 'plcontainer';

create or replace function vec(arg1 _int8) returns _int8 as
$$
# container: plc_r
arg1+1
$$ language 'plcontainer';

create or replace function vec(arg1 _int4) returns _int4 as
$$
# container: plc_r
as.integer(arg1+1)
$$ language 'plcontainer';

create or replace function vec(arg1 _numeric) returns _numeric as
$$
# container: plc_r
arg1+1
$$ language 'plcontainer';

CREATE OR REPLACE FUNCTION rintarr(arr int8[]) RETURNS int8 AS $$
# container: plc_r
return (sum(arr, na.rm=TRUE))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rintarr(arr int4[]) RETURNS int4 AS $$
# container: plc_r
return (sum(arr, na.rm=TRUE))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rintarr(arr int2[]) RETURNS int2 AS $$
# container: plc_r
return (sum(arr, na.rm=TRUE))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rfloatarr(arr float8[]) RETURNS float8 AS $$
# container: plc_r
return (sum(arr, na.rm=TRUE))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rfloatarr(arr float4[]) RETURNS float4 AS $$
# container: plc_r
return (sum(arr, na.rm=TRUE))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rfloatarr(arr numeric[]) RETURNS numeric AS $$
# container: plc_r
return (sum(arr, na.rm=TRUE))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rboolarr(arr boolean[]) RETURNS int AS $$
# container: plc_r
return (sum(arr, na.rm=TRUE))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rtimestamparr(arr timestamp[]) RETURNS timestamp[] AS $$
# container: plc_r
options(digits.secs = 6)
tmp <- strptime(arr,'%Y-%m-%d %H:%M:%OS')
return (as.character(tmp+3600))
$$
language plcontainer;


create or replace function paster(arg1 _text,arg2 _text,arg3 text) returns text[] as
$$
#container: plc_r
paste(arg1,arg2, sep = arg3)
$$
language plcontainer;

CREATE OR REPLACE FUNCTION rlog100_shared() RETURNS text AS $$
# container: plc_r_shared
return(log10(100))
$$ LANGUAGE plcontainer;

create or replace function rpg_spi_exec(arg1 text) returns text as $$
#container: plc_r
(pg.spi.exec(arg1))[[1]]
$$ language plcontainer;

CREATE OR REPLACE FUNCTION rlogging() RETURNS void AS $$
# container: plc_r
plr.debug('this is the debug message')
plr.log('this is the log message')
plr.info('this is the info message')
plr.notice('this is the notice message')
plr.warning('this is the warning message')
plr.error('this is the error message')
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION rlogging2() RETURNS void AS $$
# container: plc_r
pg.spi.exec('select rlogging()');
$$ LANGUAGE plcontainer;

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

CREATE OR REPLACE FUNCTION pynumeric(n numeric) RETURNS numeric AS $$
# container: plc_python
return n+3.0
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytimestamp(t timestamp) RETURNS timestamp AS $$
# container: plc_python
return t
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytimestamptz(t timestamptz) RETURNS timestamptz AS $$
# container: plc_python
return t
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

CREATE OR REPLACE FUNCTION pytsarr(t timestamp[]) RETURNS int AS $$
# container: plc_python
return sum([1 if '2010' in x else 0 for x in t])
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrint1(num int) RETURNS bool[] AS $BODY$
# container: plc_python
return [x for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrint2(num int) RETURNS smallint[] AS $BODY$
# container: plc_python
return [x for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrint4(num int) RETURNS int[] AS $BODY$
# container: plc_python
return [x for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrint8(num int) RETURNS int8[] AS $BODY$
# container: plc_python
return [x for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrfloat4(num int) RETURNS float4[] AS $BODY$
# container: plc_python
return [x/2.0 for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrfloat8(num int) RETURNS float8[] AS $BODY$
# container: plc_python
return [x/3.0 for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrnumeric(num int) RETURNS numeric[] AS $BODY$
# container: plc_python
return [x/4.0 for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrtext(num int) RETURNS text[] AS $BODY$
# container: plc_python
return ['number' + str(x) for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrdate(num int) RETURNS date[] AS $$
# container: plc_python
import datetime
dt = datetime.date(2016,12,31)
dts = [dt + datetime.timedelta(days=x) for x in range(num)]
return [x.strftime('%Y-%m-%d') for x in dts]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturntupint8() RETURNS int8[] AS $BODY$
# container: plc_python
return (0,1,2,3,4,5)
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrint8nulls() RETURNS int8[] AS $BODY$
# container: plc_python
return [1,2,3,None,5,6,None,8,9]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrtextnulls() RETURNS text[] AS $BODY$
# container: plc_python
return ['a','b',None,'d',None,'f']
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrmulti() RETURNS int[] AS $BODY$
# container: plc_python
return [[x for x in range(5)] for _ in range(5)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnsetofint8(num int) RETURNS setof int8 AS $BODY$
# container: plc_python
return [x for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnsetofint4arr(num int) RETURNS setof int[] AS $BODY$
# container: plc_python
return [range(x+1) for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnsetoftextarr(num int) RETURNS setof text[] AS $BODY$
# container: plc_python
def get_texts(n):
    return ['n'+str(x) for x in range(n)]
    
return [get_texts(x+1) for x in range(num)]
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnsetofdate(num int) RETURNS setof date AS $$
# container: plc_python
import datetime
dt = datetime.date(2016,12,31)
dts = [dt + datetime.timedelta(days=x) for x in range(num)]
return [x.strftime('%Y-%m-%d') for x in dts]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnsetofint8yield(num int) RETURNS setof int8 AS $BODY$
# container: plc_python
for x in range(num):
    yield x
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

CREATE OR REPLACE FUNCTION py_plpy_get_record() RETURNS int AS $$
# container: plc_python
q = """SELECT 't'::bool as a,
              1::smallint as b,
              2::int as c,
              3::bigint as d,
              4::float4 as e,
              5::float8 as f,
              6::numeric as g,
              'foobar'::varchar as h
    """
r = plpy.execute(q)
if len(r) != 1: return 1
if r[0]['a'] != 1 or str(type(r[0]['a'])) != "<type 'int'>": return 2
if r[0]['b'] != 1 or str(type(r[0]['b'])) != "<type 'int'>": return 3
if r[0]['c'] != 2 or str(type(r[0]['c'])) != "<type 'int'>": return 4
if r[0]['d'] != 3 or str(type(r[0]['d'])) != "<type 'long'>": return 5
if r[0]['e'] != 4.0 or str(type(r[0]['e'])) != "<type 'float'>": return 6
if r[0]['f'] != 5.0 or str(type(r[0]['f'])) != "<type 'float'>": return 7
if r[0]['g'] != 6.0 or str(type(r[0]['g'])) != "<type 'float'>": return 8
if r[0]['h'] != 'foobar' or str(type(r[0]['h'])) != "<type 'str'>": return 9
return 10
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylogging() RETURNS void AS $$
# container: plc_python
plpy.debug('this is the debug message')
plpy.log('this is the log message')
plpy.info('this is the info message')
plpy.notice('this is the notice message')
plpy.warning('this is the warning message')
plpy.error('this is the error message')
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylogging2() RETURNS void AS $$
# container: plc_python
plpy.execute('select pylogging()')
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pygdset(key varchar, value varchar) RETURNS text AS $$
# container: plc_python
GD[key] = value
return 'ok'
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pygdgetall() RETURNS setof text AS $$
# container: plc_python
items = sorted(GD.items())
return [ ':'.join([x[0],x[1]]) for x in items ]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pysdset(key varchar, value varchar) RETURNS text AS $$
# container: plc_python
SD[key] = value
return 'ok'
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pysdgetall() RETURNS setof text AS $$
# container: plc_python
items = sorted(SD.items())
return [ ':'.join([x[0],x[1]]) for x in items ]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyunargs1(varchar) RETURNS text AS $$
# container: plc_python
return args[0]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyunargs2(a int, varchar) RETURNS text AS $$
# container: plc_python
return str(a) + args[1]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyunargs3(a int, varchar, c varchar) RETURNS text AS $$
# container: plc_python
return str(a) + args[1] + c
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyunargs4(int, int, int, int) RETURNS int AS $$
# container: plc_python
return len(args)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylargeint8in(a int8[]) RETURNS float8 AS $$
#container : plc_python
return sum(a)/float(len(a))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylargeint8out(n int) RETURNS int8[] AS $$
#container : plc_python
return range(n)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylargetextin(t text) RETURNS int AS $$
#container : plc_python
return len(t)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylargetextout(n int) RETURNS text AS $$
#container : plc_python
return ','.join([str(x) for x in xrange(1,n+1)])
$$ LANGUAGE plcontainer;

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

CREATE OR REPLACE FUNCTION pyanaconda() RETURNS double precision AS $$
# container: plc_anaconda
import sklearn
import numpy
import scipy
import pandas
return 1.0
$$ LANGUAGE plcontainer;
