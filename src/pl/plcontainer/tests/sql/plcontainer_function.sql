/* really stupid function just to get the module loaded
*/
CREATE FUNCTION rlog100() RETURNS text AS $$
# container: plc_r
return(log10(100))
$$ LANGUAGE plcontainer;

CREATE FUNCTION writeFile() RETURNS text AS $$
# container: plc_python
f = open("/tmp/foo", "w")
f.write("foobar")
f.close
return 'wrote foobar to /tmp/foo'
$$ LANGUAGE plcontainer;

CREATE FUNCTION pylog100() RETURNS double precision AS $$
# container: plc_python
import math
return math.log10(100)
$$ LANGUAGE plcontainer;

CREATE FUNCTION pylog(a integer, b integer) RETURNS double precision AS $$
# container: plc_python
import math
return math.log(a, b)
$$ LANGUAGE plcontainer;

CREATE FUNCTION concat(a text, b text) RETURNS text AS $$
# container: plc_python
import math
return a + b
$$ LANGUAGE plcontainer;

CREATE FUNCTION concatall() RETURNS text AS $$
# container: plc_python
res = plpy.execute('select fname from users order by 1')
names = map(lambda x: x['fname'], res)
return reduce(lambda x,y: x + ',' + y, names)
$$ LANGUAGE plcontainer;

CREATE FUNCTION invalid_function() RETURNS double precision AS $$
# container: plc_python
import math
return math.foobar(a, b)
$$ LANGUAGE plcontainer;

CREATE FUNCTION invalid_syntax() RETURNS double precision AS $$
# container: plc_python
import math
return math.foobar(a,
$$ LANGUAGE plcontainer;
