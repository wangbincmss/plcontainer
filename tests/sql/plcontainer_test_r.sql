set datestyle='ISO,MDY';
select rlog100();
select rbool('t');
select rbool('f');
select rint(NULL::int2);
select rint(123::int2);
select rint(234::int4);
select rint(345::int8);
select rfloat(3.1415926535897932384626433832::float4);
select rfloat(3.1415926535897932384626433832::float8);
select rnumeric(3.1415926535897932384626433832::numeric);
select rtimestamp('2012-01-02 12:34:56.789012'::timestamp);
select rtimestamptz('2012-01-02 12:34:56.789012 UTC+4'::timestamptz);
select rtext('123');
select rtest_mia();
select vec('{1.23, 1.32}'::float8[]);
select vec('{1, 5,10, 100, 7}'::int8[]);
select vec('{1.23, 1.32}'::float4[]);
select vec('{1, 5,10, 100, 7}'::int4[]);
select vec('{1, 5,10, 100, 7}'::numeric[]);

select rintarr('{1,2,3,4}'::int2[]);
select rintarr('{1,2,3,null}'::int2[]);
select rintarr('{null}'::int2[]);
select rintarr('{1,2,3,4,5}'::int4[]);
select rintarr('{1,2,3,4,null}'::int4[]);
select rintarr('{null}'::int4[]);
select rintarr('{1,2,3,4,6}'::int8[]);
select rintarr('{1,2,3,6,null}'::int8[]);
select rintarr('{null}'::int8[]);

select rfloatarr('{1.2,2.3,3.4,5.6}'::float8[]);
select rfloatarr('{1.2,2.3,3.4,null}'::float8[]);
select rfloatarr('{null}'::float8[]);

select rfloatarr('{1.2,2.3,3.4,5.6,6.7}'::float4[]);
select rfloatarr('{1.2,2.3,3.4,5.6,null}'::float4[]);
select rfloatarr('{null}'::float4[]);

select rfloatarr('{1.2,2.3,3.4,5.6,6.7}'::numeric[]);
select rfloatarr('{1.2,2.3,3.4,5.6,null}'::numeric[]);
select rfloatarr('{null}'::numeric[]);

select rboolarr('{1,1,0}'::bool[]);
select rboolarr('{1,1,0,NULL}'::bool[]);
select rboolarr('{NULL}'::bool[]);

select rtimestamparr($${'2012-01-02 12:34:56.789012','2012-01-03 12:34:56.789012'}$$::timestamp[]);

select rlog100_shared();
select rpg_spi_exec('select 1');

select rlogging();
select rlogging2();

--select paster('{hello, happy}','{world, birthday}',' ');
--select rtest_spi_tup('select fname, lname,username from users order by 1,2,3');
-- This function is of "return setof record" type which is not supported yet
-- select rtest_spi_ta('select oid, typname from pg_type where typname = ''oid'' or typname = ''text''');
