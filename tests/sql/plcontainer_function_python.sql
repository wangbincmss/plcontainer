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

CREATE OR REPLACE FUNCTION pybytea(r bytea) RETURNS bytea AS $$
# container: plc_python
return r
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybytea2() RETURNS bytea AS $$
# container: plc_python
return None
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybytea3() RETURNS bytea AS $$
# container: plc_python
return ''
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

CREATE OR REPLACE FUNCTION pybyteaarr(b bytea[]) RETURNS bytea AS $$
# container: plc_python
return '|'.join([x.decode('UTF8') if x else 'None' for x in b])
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

CREATE OR REPLACE FUNCTION pyreturnarrbytea(a bytea[]) RETURNS bytea[] AS $BODY$
# container: plc_python
return a
$BODY$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyreturnarrbytea2(num int) RETURNS bytea[] AS $BODY$
# container: plc_python
return ['bytea' + str(x) for x in range(num)]
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
return ','.join(names)
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
import sys
q = """SELECT 't'::bool as a,
              1::smallint as b,
              2::int as c,
              3::bigint as d,
              4::float4 as e,
              5::float8 as f,
              6::numeric as g,
              'foobar'::varchar as h,
              'test'::bytea as i
    """
r = plpy.execute(q)
if len(r) != 1: return 1
# Python 2
if (sys.version_info.major == 2):
    if r[0]['a'] != 1 or str(type(r[0]['a'])) != "<type 'int'>": return 2
    if r[0]['b'] != 1 or str(type(r[0]['b'])) != "<type 'int'>": return 3
    if r[0]['c'] != 2 or str(type(r[0]['c'])) != "<type 'int'>": return 4
    if r[0]['d'] != 3 or str(type(r[0]['d'])) != "<type 'long'>": return 5
    if r[0]['e'] != 4.0 or str(type(r[0]['e'])) != "<type 'float'>": return 6
    if r[0]['f'] != 5.0 or str(type(r[0]['f'])) != "<type 'float'>": return 7
    if r[0]['g'] != 6.0 or str(type(r[0]['g'])) != "<type 'float'>": return 8
    if r[0]['h'] != 'foobar' or str(type(r[0]['h'])) != "<type 'str'>": return 9
    if r[0]['i'] != 'test' or str(type(r[0]['i'])) != "<type 'str'>": return 10
# Python 3
else:
    if r[0]['a'] != 1 or str(type(r[0]['a'])) != "<class 'int'>": return 2
    if r[0]['b'] != 1 or str(type(r[0]['b'])) != "<class 'int'>": return 3
    if r[0]['c'] != 2 or str(type(r[0]['c'])) != "<class 'int'>": return 4
    if r[0]['d'] != 3 or str(type(r[0]['d'])) != "<class 'int'>": return 5
    if r[0]['e'] != 4.0 or str(type(r[0]['e'])) != "<class 'float'>": return 6
    if r[0]['f'] != 5.0 or str(type(r[0]['f'])) != "<class 'float'>": return 7
    if r[0]['g'] != 6.0 or str(type(r[0]['g'])) != "<class 'float'>": return 8
    if r[0]['h'] != 'foobar' or str(type(r[0]['h'])) != "<class 'str'>": return 9
    if r[0]['i'].decode('UTF8') != 'test' or str(type(r[0]['i'])) != "<class 'bytes'>": return 10
return 11
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
for k in sorted(SD.keys()):
    plpy.info("SD %s -> %s" % (k, SD[k]))
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
return ','.join([str(x) for x in range(1,n+1)])
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytestudt1(r test_type) RETURNS int AS $$
# container: plc_python
import sys
# Python 2
if (sys.version_info.major == 2):
    if r['a'] != 1 or str(type(r['a'])) != "<type 'int'>": return 2
    if r['b'] != 1 or str(type(r['b'])) != "<type 'int'>": return 3
    if r['c'] != 2 or str(type(r['c'])) != "<type 'int'>": return 4
    if r['d'] != 3 or str(type(r['d'])) != "<type 'long'>": return 5
    if r['e'] != 4.0 or str(type(r['e'])) != "<type 'float'>": return 6
    if r['f'] != 5.0 or str(type(r['f'])) != "<type 'float'>": return 7
    if r['g'] != 6.0 or str(type(r['g'])) != "<type 'float'>": return 8
    if r['h'] != 'foobar' or str(type(r['h'])) != "<type 'str'>": return 9
# Python 3
else:
    if r['a'] != 1 or str(type(r['a'])) != "<class 'int'>": return 2
    if r['b'] != 1 or str(type(r['b'])) != "<class 'int'>": return 3
    if r['c'] != 2 or str(type(r['c'])) != "<class 'int'>": return 4
    if r['d'] != 3 or str(type(r['d'])) != "<class 'int'>": return 5
    if r['e'] != 4.0 or str(type(r['e'])) != "<class 'float'>": return 6
    if r['f'] != 5.0 or str(type(r['f'])) != "<class 'float'>": return 7
    if r['g'] != 6.0 or str(type(r['g'])) != "<class 'float'>": return 8
    if r['h'] != 'foobar' or str(type(r['h'])) != "<class 'str'>": return 9
return 10
$$ LANGUAGE plcontainer;


CREATE OR REPLACE FUNCTION pytestudt2(r test_type2) RETURNS int AS $$
# container: plc_python
import sys
# Python 2
if (sys.version_info.major == 2):
    if len(r['a']) != 3 or r['a'] != [1,0,1] or str(type(r['a'][0])) != "<type 'int'>": return 2
    if len(r['b']) != 3 or r['b'] != [1,2,3] or str(type(r['b'][0])) != "<type 'int'>": return 3
    if len(r['c']) != 3 or r['c'] != [2,3,4] or str(type(r['c'][0])) != "<type 'int'>": return 4
    if len(r['d']) != 3 or r['d'] != [3,4,5] or str(type(r['d'][0])) != "<type 'long'>": return 5
    if len(r['e']) != 3 or r['e'] != [4.5,5.5,6.5] or str(type(r['e'][0])) != "<type 'float'>": return 6
    if len(r['f']) != 3 or r['f'] != [5.5,6.5,7.5] or str(type(r['f'][0])) != "<type 'float'>": return 7
    if len(r['g']) != 3 or r['g'] != [6.5,7.5,8.5] or str(type(r['g'][0])) != "<type 'float'>": return 8
    if len(r['h']) != 3 or r['h'] != ['a','b','c'] or str(type(r['h'][0])) != "<type 'str'>": return 9
# Python 3
else:
    if len(r['a']) != 3 or r['a'] != [1,0,1] or str(type(r['a'][0])) != "<class 'int'>": return 2
    if len(r['b']) != 3 or r['b'] != [1,2,3] or str(type(r['b'][0])) != "<class 'int'>": return 3
    if len(r['c']) != 3 or r['c'] != [2,3,4] or str(type(r['c'][0])) != "<class 'int'>": return 4
    if len(r['d']) != 3 or r['d'] != [3,4,5] or str(type(r['d'][0])) != "<class 'int'>": return 5
    if len(r['e']) != 3 or r['e'] != [4.5,5.5,6.5] or str(type(r['e'][0])) != "<class 'float'>": return 6
    if len(r['f']) != 3 or r['f'] != [5.5,6.5,7.5] or str(type(r['f'][0])) != "<class 'float'>": return 7
    if len(r['g']) != 3 or r['g'] != [6.5,7.5,8.5] or str(type(r['g'][0])) != "<class 'float'>": return 8
    if len(r['h']) != 3 or r['h'] != ['a','b','c'] or str(type(r['h'][0])) != "<class 'str'>": return 9
return 10
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytestudt6() RETURNS test_type AS $$
# container: plc_python
return {'a': True, 'b': 1, 'c': 2, 'd': 3, 'e': 4, 'f': 5, 'g': 6, 'h': 'foo'}
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytestudt8() RETURNS SETOF test_type3 AS $$
# container: plc_python
return [{'a': 1, 'b': 2, 'c': 'foo'}, {'a': 3, 'b': 4, 'c': 'bar'}]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytestudt11() RETURNS SETOF test_type4 AS $$
# container: plc_python
return [{'a': 1, 'b': [2,22], 'c': ['foo','foo2']}, {'a': 3, 'b': [4,44], 'c': ['bar','bar2']}]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytestudt13(r test_type3) RETURNS test_type3 AS $$
# container: plc_python
return r
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytestudtrecord1() RETURNS record AS $$
# container: plc_python
return {'a': 1, 'b': 2, 'c': 'foo'}
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pytestudtrecord2() RETURNS SETOF record AS $$
# container: plc_python
return [{'a': 1, 'b': 2, 'c': 'foo'}, {'a': 3, 'b': 4, 'c': 'bar'}]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybadint() RETURNS int AS $$
# container: plc_python
return 'foo'
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybadfloat8() RETURNS float4 AS $$
# container: plc_python
return 'foo'
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybadudt() RETURNS test_type3 AS $$
# container: plc_python
return {'a': 1, 'b': 2, 'd':'foo'}
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybadudt2() RETURNS test_type4 AS $$
# container: plc_python
return {'a': 1, 'b': [1,'foo',3], 'c':['foo']}
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybadarr() RETURNS int[] AS $$
# container: plc_python
return [1,2,3,'foo',5]
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pybadarr2() RETURNS int[] AS $$
# container: plc_python
return 1
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyinvalid_function() RETURNS double precision AS $$
# container: plc_python
import math
return math.foobar(a, b)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pyinvalid_syntax() RETURNS double precision AS $$
# container: plc_python
import math
return math.foobar(a,1b)
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

CREATE OR REPLACE FUNCTION pyversion() RETURNS varchar AS $$
# container : plc_python_shared
import sys
return str(sys.version_info)
$$ LANGUAGE plcontainer;