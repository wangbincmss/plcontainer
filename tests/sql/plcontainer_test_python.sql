select pylog100();
select pylog(10000, 10);
select pybool('f');
select pybool('t');
select pyint(NULL::int2);
select pyint(123::int2);
select pyint(234::int4);
select pyint(345::int8);
select pyfloat(3.1415926535897932384626433832::float4);
select pyfloat(3.1415926535897932384626433832::float8);
select pynumeric(3.1415926535897932384626433832::numeric);
select pytimestamp('2012-01-02 12:34:56.789012'::timestamp);
select pytimestamptz('2012-01-02 12:34:56.789012 UTC+4'::timestamptz);
select pytext('text');
select pytext('');
select pybytea('123'::bytea);
select pybytea(''::bytea);
select pybytea(null::bytea);
select pybytea2() is null;
select pybytea3() is null;
select pyintarr(null::int8[]);
select pyintarr('{}'::int8[]);
select pyintarr(array[1]::int8[]);
select pyintarr(array[1,2,3,4,5]::int8[]);
select pyintarr(array[array[1,2,3,4,5],array[6,7,8,9,10]]::int8[]);
select pyfloatarr(array[1.1,2.2,3.3]::float8[]);
select pyfloatarr(array[array[1.1,3.3,5.5,7.7],array[3.3,4.4,5.5,6.6]]::float8[]);
select pytextarr(array['aaa','bbb','ccc']::varchar[]);
select pytextarr(array['aaa','','ccc']::varchar[]);
select pytsarr(array['2010-01-01 00:00:00', '2010-02-02 01:01:01', '2010-03-03 03:03:03', '2012-01-01 00:00:00']::timestamp[]);
select pybyteaarr(array['123','321']::bytea[]);
select pybyteaarr(array['123','321',null]::bytea[]);
select pybyteaarr(array[]::bytea[]);
select pyintnulls(array[1,2,3,4,5,null]::int8[]);
select pyintnulls(array[null,null,null]::int8[]);
select pyreturnarrint1(5);
select pyreturnarrint2(6);
select pyreturnarrint4(7);
select pyreturnarrint8(8);
select pyreturnarrfloat4(9);
select pyreturnarrfloat8(10);
select pyreturnarrnumeric(11);
select pyreturnarrtext(12);
select pyreturnarrdate(13);
select pyreturnarrbytea(array['foo','bar']::bytea[]);
select pyreturnarrbytea(array['foo','bar',null]::bytea[]);
select pyreturnarrbytea(null::bytea[]);
select pyreturnarrbytea2(14);
select pyreturntupint8();
select pyreturnarrint8nulls();
select pyreturnarrtextnulls();
select pyreturnarrmulti();
select pyreturnsetofint8(5);
select pyreturnsetofint4arr(6);
select pyreturnsetoftextarr(7);
select pyreturnsetofdate(8);
select pyreturnsetofint8yield(9);
select pyreturnsetofint8yield(0);
-- Test that container cannot access filesystem of the host
select pywriteFile();
\! ls -l /tmp/foo
select pyconcat(fname, lname) from users order by 1;
select pyconcatall();
select pynested_call_three('a');
select pynested_call_two('a');
select pynested_call_one('a');
select py_plpy_get_record();
select pylogging();
select pylogging2();
select pygdset('1','a');
select pygdset('2','b');
select pygdset('3','c');
select pygdgetall();
select pygdset('1','d');
select pygdgetall();
select pysdset('a','000');
select pysdset('b','111');
select pysdset('c','222');
select pysdgetall();
select pysdset('d','333');
select pysdgetall();
select pyunargs1('foo');
select pyunargs2(123, 'foo');
select pyunargs3(123, 'foo', 'bar');
select pyunargs4(1,null,null,1);
select pylargeint8in(array_agg(id)) from generate_series(1,100000) id;
select avg(x) from (select unnest(pylargeint8out(100000)) as x) as q;
select pylargetextin(string_agg(x,',')) from (select x::varchar from generate_series(1,100000) x) as q;
select length(pylargetextout(100000));
select pytestudt1( ('t', 1, 2, 3, 4, 5, 6, 'foobar')::test_type );
select pytestudt2( (
        array['t','f','t']::bool[],
        array[1,2,3]::smallint[],
        array[2,3,4]::int[],
        array[3,4,5]::int8[],
        array[4.5,5.5,6.5]::float4[],
        array[5.5,6.5,7.5]::float8[],
        array[6.5,7.5,8.5]::numeric[],
        array['a','b','c']::varchar[])::test_type2 );
select pytestudt3( array[(1,1,'a'), (2,2,'b'), (3,3,'c')]::test_type3[] );
select pytestudt4( array[
                (1,array[1,2,3],array['a','b','c']),
                (2,array[2,3,4],array['b','c','d']),
                (3,array[3,4,5],array['c','d','e'])
            ]::test_type4[] );
select pytestudt5(null::test_type4[]);
select pytestudt5(array[null]::test_type4[]);
select pytestudt6();
select pytestudt7();
select pytestudt8();
select pytestudt9();
select * from unnest(pytestudt10());
select * from pytestudt11();
select unnest(a) from (select pytestudt12() as a) as q;
select * from pytestudtrecord1() as t(a int, b int, c varchar);
select * from pytestudtrecord2() as t(a int, b int, c varchar);
select pyinvalid_function();
select pyinvalid_syntax();