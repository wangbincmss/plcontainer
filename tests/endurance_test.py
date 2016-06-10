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


QUERIES = [
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
        'select count(*) from endurance_test where plcudt1(l)::varchar != pyudt1(l)::varchar;',
        'select count(*) from endurance_test where plcudt2(m)::varchar != pyudt2(m)::varchar;',
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
        'select count(*) from endurance_test where plcabytea(x) != pyabytea(x);',
        'select count(*) from endurance_test where plcaudt1(y) is null;',
        'select count(*) from endurance_test where plcaudt2(z) is null;'
    ]


def parseargs():
    parser = OptionParser()

    parser.add_option('-d', '--dbname', type='string', default='endurance',
                      help = 'Database name to run endurance test. Would be dropped and ' +
                             'recreated by the script. Defaults to "endurance"')
    parser.add_option('-t', '--time', type='int', default='60', help='Amount of seconds dedicated for each query. Defaults to 60 seconds')
    parser.add_option('-p', '--preparesql', type='string', default='sql/plcontainer_endurance.sql',
                      help='SQL script to create endurance test database with all the data and functions')
    parser.add_option('-c', '--containersql', type='string', default='../management/sql/plcontainer_install.sql',
                      help='SQL script to create PL/Container language in given database')
    parser.add_option('-v', '--verbose', action='store_true', help='Enable verbose logging')
    

    (options, args) = parser.parse_args()
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
    if res.rc != 0:
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


def run_rand_test(dburl, runtime):
    conn = dbconn.connect(dburl)
    logger.info('Running random queries test')
    starttime = dt.datetime.now()
    execnum = 1
    while ( (dt.datetime.now() - starttime).seconds < runtime ):
        query = random.choice(QUERIES)
        logger.debug("Execution #%d query '%s'" % (execnum, query))
        res = execute(conn, query)
        if res != [[0]]:
            logger.error('Query "%s" returned incorrect result: %s' % (query, str(res)))
            return -1
        execnum += 1
    conn.close()
    return 0


def prepare(dbname, plcsql, prepsql):
    logger.info("Preparing endurance test database...")
    if execute_os("psql -d template1 -c 'drop database if exists %s'" % dbname) < 0:
        return -1
    if execute_os("psql -d template1 -c 'create database %s'" % dbname) < 0:
        return -1
    if execute_os("psql -d %s -f %s" % (dbname, plcsql)) < 0:
        return -1
    if execute_os("psql -d %s -f %s" % (dbname, prepsql)) < 0:
        return -1
    return 0


def run(dbname, runtime):
    logger.info("Running endurance test...")
    dburl = dbconn.DbURL(hostname = '127.0.0.1',
                         port     = 5432,
                         dbname   = dbname,
                         username = 'vagrant')
    run_rand_test(dburl, runtime)
    for query in QUERIES:
        if run_test(query, dburl, runtime) < 0:
            return


def main():
    options = parseargs()
    if options.verbose:
        gplog.enable_verbose_logging()
    if prepare(options.dbname, options.containersql, options.preparesql) < 0:
        return
    run(options.dbname, options.time)


if __name__ == '__main__':
    execname = os.path.split(__file__)[-1]
    gplog.setup_tool_logging(execname, getLocalHostname(), getUserName())
    logger = gplog.get_default_logger()
    main()
