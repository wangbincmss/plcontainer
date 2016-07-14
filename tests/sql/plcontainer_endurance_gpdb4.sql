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

create table endurance_test_data (
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
    x bytea[]
) distributed randomly;

create view endurance_test as
    select *
        from endurance_test_data
        where random() < 0.1;

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

insert into endurance_test_data
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
        
insert into endurance_test_data select * from endurance_test_data; -- 2000
insert into endurance_test_data select * from endurance_test_data; -- 4000
insert into endurance_test_data select * from endurance_test_data; -- 8000
insert into endurance_test_data select * from endurance_test_data; -- 16000
insert into endurance_test_data select * from endurance_test_data; -- 32000