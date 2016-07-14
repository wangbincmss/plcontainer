#!/usr/bin/env python

import random
import datetime as dt
import os
from optparse import OptionParser


try:
    from gppylib import gplog
    from gppylib.db import dbconn
    from pygresql.pg import DatabaseError
    from gppylib.commands.base import Command
    from gppylib.commands.unix import getLocalHostname, getUserName
except ImportError, e:
    sys.exit('ERROR: Cannot import Greenplum modules.  Please check that you have sourced greenplum_path.sh.  Detail: ' + str(e))


QUERIES_PYTHON = [
        'select count(*) from endurance_test where plcbool(a) != pybool(a);',
        'select count(*) from endurance_test where plcint(b) != pyint(b);',
        'select count(*) from endurance_test where plcint(c) != pyint(c);',
        'select count(*) from endurance_test where plcint(d) != pyint(d);',
        'select count(*) from endurance_test where plcfloat(e) != pyfloat(e);',
        'select count(*) from endurance_test where plcfloat(f) != pyfloat(f);',
        'select count(*) from endurance_test where plcnumeric(g) != pynumeric(g);',
        'select count(*) from endurance_test where plctimestamp(h) != pytimestamp(h);',
        'select count(*) from endurance_test where plcvarchar(i) != pyvarchar(i);',
        'select count(*) from endurance_test where plctext(j) != pytext(j);',
        'select count(*) from endurance_test where plcbytea(k) != pybytea(k);',
        'select count(*) from endurance_test where plcudt1(l) is null;',
        'select count(*) from endurance_test where plcudt2(m) is null;',
        'select count(*) from endurance_test where plcabool(n) != pyabool(n);',
        'select count(*) from endurance_test where plcaint(o) != pyaint(o);',
        'select count(*) from endurance_test where plcaint(p) != pyaint(p);',
        'select count(*) from endurance_test where plcaint(q) != pyaint(q);',
        'select count(*) from endurance_test where plcafloat(r) != pyafloat(r);',
        'select count(*) from endurance_test where plcafloat(s) != pyafloat(s);',
        'select count(*) from endurance_test where plcanumeric(t) != pyanumeric(t);',
        'select count(*) from endurance_test where plcatimestamp(u) != pyatimestamp(u);',
        'select count(*) from endurance_test where plcavarchar(v) != pyavarchar(v);',
        'select count(*) from endurance_test where plcatext(w) != pyatext(w);',
        'select count(*) from endurance_test where plcabytea(x) != pyabytea(x);'
    ]
QUERIES_PYTHON_5 = [
        'select count(*) from endurance_test where random() < 0.1 and plcaudt1(y) is null;',
        'select count(*) from endurance_test where random() < 0.1 and plcaudt2(z) is null;',
        'select count(*) from (select plcsudt1(y) as y from endurance_test where random() < 0.1) as q where y is null;',
        'select count(*) from (select plcsudt2(z) as z from endurance_test where random() < 0.1) as q where z is null;'
    ]

QUERIES_R = [
        'select count(*) from endurance_test where rbool(a) is null;',
        'select count(*) from endurance_test where rint(b) is null;',
        'select count(*) from endurance_test where rint(c) is null;',
        'select count(*) from endurance_test where rint(d) is null;',
        'select count(*) from endurance_test where rfloat(e) is null;',
        'select count(*) from endurance_test where rfloat(f) is null;',
        'select count(*) from endurance_test where rnumeric(g) is null;',
        'select count(*) from endurance_test where rtimestamp(h) is null;',
        'select count(*) from endurance_test where rvarchar(i) is null;',
        'select count(*) from endurance_test where rtext(j) is null;',
        'select count(*) from endurance_test where rudt1(l) is null;',
        'select count(*) from endurance_test where rudt2(m) is null;',
        'select count(*) from endurance_test where rabool(n) is null;',
        'select count(*) from endurance_test where raint(o) is null;',
        'select count(*) from endurance_test where raint(p) is null;',
        'select count(*) from endurance_test where raint(q) is null;',
        'select count(*) from endurance_test where rafloat(r) is null;',
        'select count(*) from endurance_test where rafloat(s) is null;',
        'select count(*) from endurance_test where ranumeric(t) is null;',
        'select count(*) from endurance_test where ratimestamp(u) is null;',
        'select count(*) from endurance_test where ravarchar(v) is null;',
        'select count(*) from endurance_test where ratext(w) is null;',
        'select count(*) from endurance_test where rbyteaout1(l) is null;',
        'select count(*) from endurance_test where rbyteain1(rbyteaout1(l)) is null;',
        'select count(*) from endurance_test where rbyteaout2(m) is null;',
        'select count(*) from endurance_test where rbyteain2(rbyteaout2(m)) is null;'
    ]
QUERIES_R_5 = [
        'select count(*) from endurance_test where random() < 0.1 and raudt1(y) is null;',
        'select count(*) from endurance_test where random() < 0.1 and raudt2(z) is null;',
        'select count(*) from (select rsudt1(y) as y from endurance_test where random() < 0.1) as q where y is null;',
        'select count(*) from (select rsudt2(z) as z from endurance_test where random() < 0.1) as q where z is null;',
        'select count(*) from endurance_test where random() < 0.1 and rbyteaout3(y) is null;',
        'select count(*) from endurance_test where random() < 0.1 and rbyteain3(rbyteaout3(y)) is null;',
        'select count(*) from endurance_test where random() < 0.1 and rbyteaout4(z) is null;',
        'select count(*) from endurance_test where random() < 0.1 and rbyteain4(rbyteaout4(z)) is null;'
    ]


def parseargs():
    parser = OptionParser()

    parser.add_option('-g', '--gpdbgen', type='string', default='5',
                      help='GPDB Engine generation - should be either 4 or 5')
    parser.add_option('-d', '--dbname', type='string', default='endurance',
                      help = 'Database name to run endurance test. Would be dropped and ' +
                             'recreated by the script. Defaults to "endurance"')
    parser.add_option('-u', '--username', type='string', default='vagrant',
                      help='User name used to connect to the database')
    parser.add_option('-t', '--time', type='int', default='60', help='Amount of seconds dedicated for each query. Defaults to 60 seconds')
    parser.add_option('-s', '--sqldir', type='string', default='sql',
                      help='Directory containing SQL scripts plcontainer_install.sql and plcontainer_endurance_gpdb*.sql')
    parser.add_option('--nopython', action='store_true', help='Avoid running PL/Container Python test')
    parser.add_option('--nopython3', action='store_true', help='Avoid running PL/Container Python3 test')
    parser.add_option('--nor', action='store_true', help='Avoid running PL/Container R test')
    parser.add_option('-v', '--verbose', action='store_true', help='Enable verbose logging')

    (options, args) = parser.parse_args()
    if options.nopython and options.nor and options.nopython3:
        logger.error("You cannot specify all --nopython, --nopython3 and --nor at the same time")
        sys.exit(3)
    if not options.gpdbgen or not options.gpdbgen in ['4', '5']:
        logger.error("You must specify --gpdbgen | -g parameter to the GPDB version and it should be either 4 or 5")
        sys.exit(3)
    return options


def execute(conn, query):
    res = []
    try:
        curs = dbconn.execSQL(conn, query)
        res = curs.fetchall()
        conn.commit()
    except DatabaseError, ex:
        logger.error('Failed to execute the statement on the database. Please, check log file for errors.')
        logger.error(ex)
    return res


def execute_os(command):
    cmd = Command("Endurance test command", command)
    cmd.run(validateAfter=True)
    res = cmd.get_results()
    if res.rc != 0 or 'ERROR' in res.stderr:
        logger.error("Error executing OS command '%s'" % command)
        logger.error("STDOUT:")
        logger.error(res.stdout)
        logger.error("STDERR:")
        logger.error(res.stderr)
        return -1
    else:
        logger.debug(res.stdout)
    return 0


def run_test(query, dburl, runtime):
    conn = dbconn.connect(dburl)
    logger.info('Running query "%s"' % query)
    starttime = dt.datetime.now()
    execnum = 1
    while ( (dt.datetime.now() - starttime).seconds < runtime ):
        logger.debug("Execution #%d" % execnum)
        res = execute(conn, query)
        if res != [[0]]:
            logger.error('Query returned incorrect result: %s' % str(res))
            return -1
        execnum += 1
    conn.close()
    return 0


def run_rand_test(dburl, runtime, queries):
    conn = dbconn.connect(dburl)
    logger.info('Running random queries test')
    starttime = dt.datetime.now()
    execnum = 1
    while ( (dt.datetime.now() - starttime).seconds < runtime ):
        query = random.choice(queries)
        logger.debug("Execution #%d query '%s'" % (execnum, query))
        res = execute(conn, query)
        if res != [[0]]:
            logger.error('Query "%s" returned incorrect result: %s' % (query, str(res)))
            return -1
        execnum += 1
    conn.close()
    return 0


def prepare(dbname, sqldir, gpdbgen):
    logger.info("Preparing endurance test data...")
    if execute_os("psql -d template1 -c 'drop database if exists %s'" % dbname) < 0:
        return -1
    if execute_os("psql -d template1 -c 'create database %s'" % dbname) < 0:
        return -1
    if execute_os("psql -d %s -f %s/plcontainer_install.sql" % (dbname, sqldir)) < 0:
        return -1
    if execute_os("psql -d %s -f %s/plcontainer_endurance_gpdb%s.sql" % (dbname, sqldir, gpdbgen)) < 0:
        return -1
    return 0

def prepare_funcs(ctype, dbname, sqldir, gpdbgen):
    logger.info("Preparing endurance test functions for '%s' test..." % ctype)
    if ctype == 'python3':
        if execute_os(("sed 's/plc_python/plc_python3/g' %s/plcontainer_endurance_functions_gpdb%s.sql" +
                      " > /tmp/plcontainer_endurance_functions_gpdb%s.sql") % (sqldir, gpdbgen, gpdbgen)) < 0:
            return -1
        sqldir = '/tmp'
    if execute_os("psql -d %s -f %s/plcontainer_endurance_functions_gpdb%s.sql" % (dbname, sqldir, gpdbgen)) < 0:
        return -1
    return 0

def run(dbname, username, runtime, queries, ctype):
    logger.info("Running endurance test for '%s' language..." % ctype)
    dburl = dbconn.DbURL(hostname = '127.0.0.1',
                         port     = 5432,
                         dbname   = dbname,
                         username = username)
    logger.info("Single queries...")
    for query in queries:
        if run_test(query, dburl, runtime) < 0:
            return
    logger.info("Random queries...")
    run_rand_test(dburl, runtime, queries)


def main():
    options = parseargs()
    if options.verbose:
        gplog.enable_verbose_logging()
    if prepare(options.dbname, options.sqldir, options.gpdbgen) < 0:
        return
    if not options.nopython:
        queries = QUERIES_PYTHON
        if options.gpdbgen == '5':
            queries += QUERIES_PYTHON_5
        if prepare_funcs("python", options.dbname, options.sqldir, options.gpdbgen) < 0:
            return
        run(options.dbname, options.username, options.time, queries, "python")
    if not options.nopython3:
        queries = QUERIES_PYTHON
        if options.gpdbgen == '5':
            queries += QUERIES_PYTHON_5
        if prepare_funcs("python3", options.dbname, options.sqldir, options.gpdbgen) < 0:
            return
        run(options.dbname, options.username, options.time, queries, "python3")
    if not options.nor:
        queries = QUERIES_R
        if options.gpdbgen == '5':
            queries += QUERIES_R_5
        if prepare_funcs("R", options.dbname, options.sqldir, options.gpdbgen) < 0:
            return
        run(options.dbname, options.username, options.time, queries, "R")


if __name__ == '__main__':
    execname = os.path.split(__file__)[-1]
    gplog.setup_tool_logging(execname, getLocalHostname(), getUserName())
    logger = gplog.get_default_logger()
    main()
