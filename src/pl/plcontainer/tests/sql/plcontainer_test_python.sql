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
select pytext('text');
-- Test that container cannot access filesystem of the host
select pywriteFile();
\! ls -l /tmp/foo
select pyconcat(fname, lname) from users order by 1;
select pyconcatall();
select pynested_call_three('a');
select pynested_call_two('a');
select pynested_call_one('a');
select pyinvalid_function();
select pyinvalid_syntax();
select pylog100_shared();