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