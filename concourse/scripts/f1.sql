CREATE OR REPLACE FUNCTION pylog100() RETURNS double precision AS $$            
# container: plc_python_shared                                                         
import math                                                                     
return math.log10(100)                                                          
$$ LANGUAGE plcontainer;

select pylog100();
