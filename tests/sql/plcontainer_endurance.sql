/* ======================================================================== */
/* Test structures and data */
/* ======================================================================== */

create language plpythonu;

create type endurance_udt1 as (
    a int,
    b float8,
    c varchar
);

create type endurance_udt2 as (
    a int[],
    b float8[],
    c varchar[]
);

create table endurance_test (
    a bool,
    b smallint,
    c int,
    d bigint,
    e float4,
    f float8,
    g numeric,
    h timestamp,
    i varchar,
    j text,
    k bytea,
    l endurance_udt1,
    m endurance_udt2,
    n bool[],
    o smallint[],
    p int[],
    q bigint[],
    r float4[],
    s float8[],
    t numeric[],
    u timestamp[],
    v varchar[],
    w text[],
    x bytea[],
    y endurance_udt1[],
    z endurance_udt2[]
) distributed randomly;

create or replace function generate_rows(nrows int) returns setof record as $BODY$
import random
import time
from datetime import datetime

def randstr(n):
    s = 'abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()[]'
    return ''.join( [random.choice(s) for _ in range(n)] )

def getrow():
    return [
        True if random.random() > 0.5 else False,
        int(random.random() * 100.0),
        int(random.random() * 1000000000.0),
        int(random.random() * 1000000000000000.0),
        random.random() * 10000000.0,
        random.random() * 100000000000.0,
        random.random() * 100000000000.0,
        datetime.utcfromtimestamp(time.time()*random.random()).strftime('%Y-%m-%d %H:%M:%S'),
        randstr(100),
        randstr(1000),
        randstr(200),
        {'a': random.randint(1,100), 'b': random.random(), 'c': randstr(10)},
        {'a': [random.randint(1,100) for _ in range(random.randint(1,100))],
         'b': [random.random() for _ in range(random.randint(1,100))],
         'c': [randstr(random.randint(1,50)) for _ in range(random.randint(1,100))]}
        ]

for _ in range(nrows):
    row = getrow()
    rows = [getrow() for _ in range(random.randint(1,10))]
    frow = row + zip(*rows)
    frow = frow[:-2]
    yield dict( zip( list('abcdefghijklmnopqrstuvwx'), frow ) )
$BODY$ language plpythonu volatile;


create or replace function generate_udt1() returns endurance_udt1 as $BODY$
import random

def randstr(n):
    s = 'abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()[]'
    return ''.join( [random.choice(s) for _ in range(n)] )

return {'a': random.randint(1,100), 'b': random.random(), 'c': randstr(10)}
$BODY$ language plpythonu volatile;


create or replace function generate_udt2() returns endurance_udt2 as $BODY$
import random

def randstr(n):
    s = 'abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()[]'
    return ''.join( [random.choice(s) for _ in range(n)] )

return {'a': [random.randint(1,100) for _ in range(random.randint(1,100))],
        'b': [random.random() for _ in range(random.randint(1,100))],
        'c': [randstr(random.randint(1,50)) for _ in range(random.randint(1,100))]}
$BODY$ language plpythonu volatile;


insert into endurance_test
    select * from generate_rows(1000) as tt (
        a bool,
        b smallint,
        c int,
        d bigint,
        e float4,
        f float8,
        g numeric,
        h timestamp,
        i varchar,
        j text,
        k bytea,
        l endurance_udt1,
        m endurance_udt2,
        n bool[],
        o smallint[],
        p int[],
        q bigint[],
        r float4[],
        s float8[],
        t numeric[],
        u timestamp[],
        v varchar[],
        w text[],
        x bytea[]);

update endurance_test as et
    set y = q.y,
        z = q.z
    from (
            select  b,
                    array_agg(generate_udt1()) as y,
                    array_agg(generate_udt2()) as z
                from endurance_test
                group by b
        ) as q
    where q.b = et.b;
        
    insert into endurance_test select * from endurance_test; -- 2000
    insert into endurance_test select * from endurance_test; -- 4000
    insert into endurance_test select * from endurance_test; -- 8000
    insert into endurance_test select * from endurance_test; -- 16000
    insert into endurance_test select * from endurance_test; -- 32000

/* ======================================================================== */
/* PL/Container Python functions */
/* ======================================================================== */

CREATE OR REPLACE FUNCTION plcbool(b bool) RETURNS bool AS $$
# container: plc_python
return b
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcint(i int2) RETURNS int2 AS $$
# container: plc_python
return i
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcint(i int4) RETURNS int4 AS $$
# container: plc_python
return i
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcint(i int8) RETURNS int8 AS $$
# container: plc_python
return i
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcfloat(f float4) RETURNS float4 AS $$
# container: plc_python
return f
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcfloat(f float8) RETURNS float8 AS $$
# container: plc_python
return f
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcnumeric(n numeric) RETURNS numeric AS $$
# container: plc_python
return n
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plctimestamp(t timestamp) RETURNS timestamp AS $$
# container: plc_python
return t
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcvarchar(t varchar) RETURNS varchar AS $$
# container: plc_python
return t
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plctext(t text) RETURNS text AS $$
# container: plc_python
return t
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcbytea(r bytea) RETURNS bytea AS $$
# container: plc_python
return r
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcudt1(u endurance_udt1) RETURNS endurance_udt1 AS $$
# container: plc_python
return u
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcudt2(u endurance_udt2) RETURNS endurance_udt2 AS $$
# container: plc_python
return u
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcabool(b bool[]) RETURNS bool[] AS $$
# container: plc_python
return b
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcaint(i int2[]) RETURNS int2[] AS $$
# container: plc_python
return i
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcaint(i int4[]) RETURNS int4[] AS $$
# container: plc_python
return i
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcaint(i int8[]) RETURNS int8[] AS $$
# container: plc_python
return i
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcafloat(f float4[]) RETURNS float4[] AS $$
# container: plc_python
return f
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcafloat(f float8[]) RETURNS float8[] AS $$
# container: plc_python
return f
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcanumeric(n numeric[]) RETURNS numeric[] AS $$
# container: plc_python
return n
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcatimestamp(t timestamp[]) RETURNS timestamp[] AS $$
# container: plc_python
return t
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcavarchar(t varchar[]) RETURNS varchar[] AS $$
# container: plc_python
return t
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcatext(t text[]) RETURNS text[] AS $$
# container: plc_python
return t
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcabytea(r bytea[]) RETURNS bytea[] AS $$
# container: plc_python
return r
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcaudt1(u endurance_udt1[]) RETURNS endurance_udt1[] AS $$
# container: plc_python
return u
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcaudt2(u endurance_udt2[]) RETURNS endurance_udt2[] AS $$
# container: plc_python
return u
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcsudt1(u endurance_udt1[]) RETURNS SETOF endurance_udt1 AS $$
# container: plc_python
return u
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION plcsudt2(u endurance_udt2[]) RETURNS SETOF endurance_udt2 AS $$
# container: plc_python
return u
$$ LANGUAGE plcontainer volatile;

/* ======================================================================== */
/* Untrusted PL/Python functions */
/* ======================================================================== */

CREATE OR REPLACE FUNCTION pybool(b bool) RETURNS bool AS $$
return b
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyint(i int2) RETURNS int2 AS $$
return i
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyint(i int4) RETURNS int4 AS $$
return i
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyint(i int8) RETURNS int8 AS $$
return i
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyfloat(f float4) RETURNS float4 AS $$
return f
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyfloat(f float8) RETURNS float8 AS $$
return f
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pynumeric(n numeric) RETURNS numeric AS $$
return n
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pytimestamp(t timestamp) RETURNS timestamp AS $$
return t
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyvarchar(t varchar) RETURNS varchar AS $$
return t
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pytext(t text) RETURNS text AS $$
return t
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pybytea(r bytea) RETURNS bytea AS $$
return r
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyudt1(u endurance_udt1) RETURNS endurance_udt1 AS $$
return u
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyudt2(u endurance_udt2) RETURNS endurance_udt2 AS $$
return u
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyabool(b bool[]) RETURNS bool[] AS $$
return b
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyaint(i int2[]) RETURNS int2[] AS $$
return i
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyaint(i int4[]) RETURNS int4[] AS $$
return i
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyaint(i int8[]) RETURNS int8[] AS $$
return i
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyafloat(f float4[]) RETURNS float4[] AS $$
return f
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyafloat(f float8[]) RETURNS float8[] AS $$
return f
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyanumeric(n numeric[]) RETURNS numeric[] AS $$
return n
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyatimestamp(t timestamp[]) RETURNS timestamp[] AS $$
return t
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyavarchar(t varchar[]) RETURNS varchar[] AS $$
return t
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyatext(t text[]) RETURNS text[] AS $$
return t
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyabytea(r bytea[]) RETURNS bytea[] AS $$
return r
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyaudt1(u endurance_udt1[]) RETURNS endurance_udt1[] AS $$
return u
$$ LANGUAGE plpythonu volatile;

CREATE OR REPLACE FUNCTION pyaudt2(u endurance_udt2[]) RETURNS endurance_udt2[] AS $$
return u
$$ LANGUAGE plpythonu volatile;

/* Queries used for testing Python:
select count(*) from endurance_test where plcbool(a) != pybool(a);
select count(*) from endurance_test where plcint(b) != pyint(b);
select count(*) from endurance_test where plcint(c) != pyint(c);
select count(*) from endurance_test where plcint(d) != pyint(d);
select count(*) from endurance_test where plcfloat(e) != pyfloat(e);
select count(*) from endurance_test where plcfloat(f) != pyfloat(f);
select count(*) from endurance_test where plcnumeric(g) != pynumeric(g);
select count(*) from endurance_test where plctimestamp(h) != pytimestamp(h);
select count(*) from endurance_test where plcvarchar(i) != pyvarchar(i);
select count(*) from endurance_test where plctext(j) != pytext(j);
select count(*) from endurance_test where plcbytea(k) != pybytea(k);
select count(*) from endurance_test where plcudt1(l)::varchar != pyudt1(l)::varchar;
select count(*) from endurance_test where plcudt2(m)::varchar != pyudt2(m)::varchar;
select count(*) from endurance_test where plcabool(n) != pyabool(n);
select count(*) from endurance_test where plcaint(o) != pyaint(o);
select count(*) from endurance_test where plcaint(p) != pyaint(p);
select count(*) from endurance_test where plcaint(q) != pyaint(q);
select count(*) from endurance_test where plcafloat(r) != pyafloat(r);
select count(*) from endurance_test where plcafloat(s) != pyafloat(s);
select count(*) from endurance_test where plcanumeric(t) != pyanumeric(t);
select count(*) from endurance_test where plcatimestamp(u) != pyatimestamp(u);
select count(*) from endurance_test where plcavarchar(v) != pyavarchar(v);
select count(*) from endurance_test where plcatext(w) != pyatext(w);
select count(*) from endurance_test where plcabytea(x) != pyabytea(x);
select count(*) from endurance_test where plcaudt1(y) is null;
select count(*) from endurance_test where plcaudt2(z) is null;
select count(*) from (select plcsudt1(y) as y from endurance_test) as q where y is null;
select count(*) from (select plcsudt2(z) as z from endurance_test) as q where z is null;
*/

/* ======================================================================== */
/* PL/Container R functions */
/* ======================================================================== */

CREATE OR REPLACE FUNCTION rbool(b bool) RETURNS bool AS $$
# container: plc_r
return (b)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rint(i int2) RETURNS int2 AS $$
# container: plc_r
return (i)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rint(i int4) RETURNS int4 AS $$
# container: plc_r
return (i)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rint(i int8) RETURNS int8 AS $$
# container: plc_r
return (i)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rfloat(f float4) RETURNS float4 AS $$
# container: plc_r
return (f)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rfloat(f float8) RETURNS float8 AS $$
# container: plc_r
return (f)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rnumeric(n numeric) RETURNS numeric AS $$
# container: plc_r
return (n)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rtimestamp(t timestamp) RETURNS timestamp AS $$
# container: plc_r
return (t)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rvarchar(t varchar) RETURNS varchar AS $$
# container: plc_r
return (t)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rtext(t text) RETURNS text AS $$
# container: plc_r
return (t)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rbytea(r bytea) RETURNS bytea AS $$
# container: plc_r
return (r)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rudt1(u endurance_udt1) RETURNS endurance_udt1 AS $$
# container: plc_r
return (u)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rudt2(u endurance_udt2) RETURNS endurance_udt2 AS $$
# container: plc_r
return (u)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rabool(b bool[]) RETURNS bool[] AS $$
# container: plc_r
return (b)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION raint(i int2[]) RETURNS int2[] AS $$
# container: plc_r
return (i)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION raint(i int4[]) RETURNS int4[] AS $$
# container: plc_r
return (i)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION raint(i int8[]) RETURNS int8[] AS $$
# container: plc_r
return (i)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rafloat(f float4[]) RETURNS float4[] AS $$
# container: plc_r
return (f)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rafloat(f float8[]) RETURNS float8[] AS $$
# container: plc_r
return (f)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION ranumeric(n numeric[]) RETURNS numeric[] AS $$
# container: plc_r
return (n)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION ratimestamp(t timestamp[]) RETURNS timestamp[] AS $$
# container: plc_r
return (t)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION ravarchar(t varchar[]) RETURNS varchar[] AS $$
# container: plc_r
return (t)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION ratext(t text[]) RETURNS text[] AS $$
# container: plc_r
return (t)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rabytea(r bytea[]) RETURNS bytea[] AS $$
# container: plc_r
return (r)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION raudt1(u endurance_udt1[]) RETURNS endurance_udt1[] AS $$
# container: plc_r
return (u)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION raudt2(u endurance_udt2[]) RETURNS endurance_udt2[] AS $$
# container: plc_r
return (u)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rsudt1(u endurance_udt1[]) RETURNS SETOF endurance_udt1 AS $$
# container: plc_r
return (u)
$$ LANGUAGE plcontainer volatile;

CREATE OR REPLACE FUNCTION rsudt2(u endurance_udt2[]) RETURNS SETOF endurance_udt2 AS $$
# container: plc_r
return (u)
$$ LANGUAGE plcontainer volatile;

create or replace function rbyteaout1(arg endurance_udt1) returns bytea as $$
# container: plc_r
return (arg)
$$ language plcontainer;

create or replace function rbyteain1(arg bytea) returns endurance_udt1 as $$
# container: plc_r
return (arg)
$$ language plcontainer;

create or replace function rbyteaout2(arg endurance_udt2) returns bytea as $$
# container: plc_r
return (arg)
$$ language plcontainer;

create or replace function rbyteain2(arg bytea) returns endurance_udt2 as $$
# container: plc_r
return (arg)
$$ language plcontainer;

create or replace function rbyteaout3(arg endurance_udt1[]) returns bytea as $$
# container: plc_r
return (arg)
$$ language plcontainer;

create or replace function rbyteain3(arg bytea) returns endurance_udt1[] as $$
# container: plc_r
return (arg)
$$ language plcontainer;

create or replace function rbyteaout4(arg endurance_udt2[]) returns bytea as $$
# container: plc_r
return (arg)
$$ language plcontainer;

create or replace function rbyteain4(arg bytea) returns endurance_udt2[] as $$
# container: plc_r
return (arg)
$$ language plcontainer;


/* Queries used for testing R:
select count(*) from endurance_test where rbool(a) is null;
select count(*) from endurance_test where rint(b) is null;
select count(*) from endurance_test where rint(c) is null;
select count(*) from endurance_test where rint(d) is null;
select count(*) from endurance_test where rfloat(e) is null;
select count(*) from endurance_test where rfloat(f) is null;
select count(*) from endurance_test where rnumeric(g) is null;
select count(*) from endurance_test where rtimestamp(h) is null;
select count(*) from endurance_test where rvarchar(i) is null;
select count(*) from endurance_test where rtext(j) is null;
select count(*) from endurance_test where rudt1(l) is null;
select count(*) from endurance_test where rudt2(m) is null;
select count(*) from endurance_test where rabool(n) is null;
select count(*) from endurance_test where raint(o) is null;
select count(*) from endurance_test where raint(p) is null;
select count(*) from endurance_test where raint(q) is null;
select count(*) from endurance_test where rafloat(r) is null;
select count(*) from endurance_test where rafloat(s) is null;
select count(*) from endurance_test where ranumeric(t) is null;
select count(*) from endurance_test where ratimestamp(u) is null;
select count(*) from endurance_test where ravarchar(v) is null;
select count(*) from endurance_test where ratext(w) is null;
select count(*) from endurance_test where raudt1(y) is null;
select count(*) from endurance_test where raudt2(z) is null;
select count(*) from (select rsudt1(y) as y from endurance_test) as q where y is null;
select count(*) from (select rsudt2(z) as z from endurance_test) as q where z is null;
select count(*) from endurance_test where rbyteaout1(l) is null;
select count(*) from endurance_test where rbyteain1(rbyteaout1(l)) is null;
select count(*) from endurance_test where rbyteaout2(m) is null;
select count(*) from endurance_test where rbyteain2(rbyteaout2(m)) is null;
select count(*) from endurance_test where rbyteaout3(y) is null;
select count(*) from endurance_test where rbyteain3(rbyteaout3(y)) is null;
select count(*) from endurance_test where rbyteaout4(z) is null;
select count(*) from endurance_test where rbyteain4(rbyteaout4(z)) is null;
*/

















