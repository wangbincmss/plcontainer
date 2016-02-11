/* really stupid function just to get the module loaded
*/
CREATE FUNCTION rlog100() RETURNS text AS $$
# container: plr
return(log10(100))
$$ LANGUAGE plspython;

CREATE FUNCTION writeFile() RETURNS text AS $$
# container: plspython
f = open("/tmp/foo", "w")
f.write("foobar")
f.close
return 'wrote foobar to /tmp/foo'
$$ LANGUAGE plspython;

CREATE FUNCTION pylog100() RETURNS double precision AS $$
# container: plspython
import math
return math.log10(100)
$$ LANGUAGE plspython;

CREATE FUNCTION pylog(a integer, b integer) RETURNS double precision AS $$
# container: plspython
import math
return math.log(a, b)
$$ LANGUAGE plspython;

CREATE FUNCTION concat(a text, b text) RETURNS text AS $$
# container: plspython
import math
return a + b
$$ LANGUAGE plspython;

CREATE FUNCTION concatall() RETURNS text AS $$
# container: plspython
res = plpy.execute('select fname from users')
names = map(lambda x: x['fname'], res)
return reduce(lambda x,y: x + ',' + y, names)
$$ LANGUAGE plspython;

CREATE FUNCTION invalid_function() RETURNS double precision AS $$
# container: plspython
import math
return math.foobar(a, b)
$$ LANGUAGE plspython;

CREATE FUNCTION invalid_syntax() RETURNS double precision AS $$
# container: plspython
import math
return math.foobar(a,
$$ LANGUAGE plspython;

