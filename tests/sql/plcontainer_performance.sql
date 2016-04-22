create language plcontainer;
create language plpythonu;

CREATE or replace FUNCTION pylog100() RETURNS double precision AS $$
import math
return math.log10(100)
$$ LANGUAGE plpythonu volatile;

CREATE or replace FUNCTION spylog100() RETURNS double precision AS $$
# container: plc_python
import math
return math.log10(100)
$$ LANGUAGE plspython volatile;

CREATE or replace FUNCTION pyre() RETURNS varchar AS $$
import re
regexp = re.compile('(?P<dt>[0-9]{4}-[0-9]{2}-[0-9]{2})T(?P<tm>[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]+)[\+\-][0-9]+\:\s+[0-9]+\.[0-9]+:\s+\[Full GC\s+\[PSYoungGen: (?P<ygos>[0-9]+)K->(?P<ygns>[0-9]+)K\((?P<ygms>[0-9]+)K\)\]\s+\[PSOldGen: (?P<ogos>[0-9]+)K->(?P<ogns>[0-9]+)K\((?P<ogms>[0-9]+)K\)\]\s+(?P<fhos>[0-9]+)K->(?P<fhns>[0-9]+)K\((?P<fhms>[0-9]+)K\)\s+\[PSPermGen: (?P<pgos>[0-9]+)K->(?P<pgns>[0-9]+)K\((?P<pgms>[0-9]+)K\)\],\s(?P<gs>[0-9]+\.[0-9]+)\s+secs\]')
regexp.match('2015-10-08T03:09:53.854+0000: 60641.076: [Full GC [PSYoungGen: 223266K->0K(940736K)] [PSOldGen: 2175774K->528591K(2470592K)] 2399040K->528591K(3411328K) [PSPermGen: 30633K->30473K(63360K)], 1.3265890 secs] [Times: user=1.32 sys=0.00, real=1.33 secs]')
return ''
$$ LANGUAGE plpythonu volatile;

CREATE or replace FUNCTION spyre() RETURNS varchar AS $$
# container: plc_python
import re
regexp = re.compile('(?P<dt>[0-9]{4}-[0-9]{2}-[0-9]{2})T(?P<tm>[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]+)[\+\-][0-9]+\:\s+[0-9]+\.[0-9]+:\s+\[Full GC\s+\[PSYoungGen: (?P<ygos>[0-9]+)K->(?P<ygns>[0-9]+)K\((?P<ygms>[0-9]+)K\)\]\s+\[PSOldGen: (?P<ogos>[0-9]+)K->(?P<ogns>[0-9]+)K\((?P<ogms>[0-9]+)K\)\]\s+(?P<fhos>[0-9]+)K->(?P<fhns>[0-9]+)K\((?P<fhms>[0-9]+)K\)\s+\[PSPermGen: (?P<pgos>[0-9]+)K->(?P<pgns>[0-9]+)K\((?P<pgms>[0-9]+)K\)\],\s(?P<gs>[0-9]+\.[0-9]+)\s+secs\]')
regexp.match('2015-10-08T03:09:53.854+0000: 60641.076: [Full GC [PSYoungGen: 223266K->0K(940736K)] [PSOldGen: 2175774K->528591K(2470592K)] 2399040K->528591K(3411328K) [PSPermGen: 30633K->30473K(63360K)], 1.3265890 secs] [Times: user=1.32 sys=0.00, real=1.33 secs]')
return ''
$$ LANGUAGE plspython volatile;

\timing
\echo ========= TEST 1 - log10 ===========
\echo PLPYTHON
\echo ========== 10000 calls =============
select count(*) from (select  pylog100() from generate_series(1,10000)) as q;
\echo ========== 20000 calls =============
select count(*) from (select  pylog100() from generate_series(1,20000)) as q;
\echo ========== 30000 calls =============
select count(*) from (select  pylog100() from generate_series(1,30000)) as q;
\echo ====================================
\echo PYTHON-ON-DOCKER
\echo ========== 10000 calls =============
select count(*) from (select spylog100() from generate_series(1,10000)) as q;
\echo ========== 20000 calls =============
select count(*) from (select spylog100() from generate_series(1,20000)) as q;
\echo ========== 30000 calls =============
select count(*) from (select spylog100() from generate_series(1,30000)) as q;

\echo ========= TEST 2 - regex ===========
\echo ====================================
\echo TESTING UNTRUSTED PYTHON PERFORMANCE
select count(*) from (select  pyre() from generate_series(1,10000)) as q;
\echo ====================================
\echo TESTING PYTHON-ON-DOCKER PERFORMANCE
select count(*) from (select spyre() from generate_series(1,10000)) as q;
