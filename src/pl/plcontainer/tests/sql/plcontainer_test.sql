-- first some tests of basic functionality
--
-- better succeed
--
select writeFile();
-- make sure the file doesn't exist on the host filesystem
\! ls -l /tmp/foo

select rlog100();
select pg_spi_exec('select 1');
select pyint(NULL::int2);
select pyint(1::int2);
select pyint(1::int4);
select pyint(1::int8);
select pyfloat(1.2::float4);
select pyfloat(1.2::float8);
select pybool('f');
select pybool('t');
select pylog100();
select pylog(10000, 10);
select concat(fname, lname) from users order by 1;
select concatall();
select nested_call_three('a');
select nested_call_two('a');
select nested_call_one('a');
select invalid_function();
select invalid_syntax();
select rlog100_shared();
select pylog100_shared();