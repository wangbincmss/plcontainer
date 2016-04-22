import datetime as dt
from gppylib.db import dbconn
from pygresql.pg import DatabaseError

def execute_noret(dburl, query):
    try:
        conn = dbconn.connect(dburl)
        curs = dbconn.execSQL(conn, query)
        conn.commit()
        conn.close()
    except DatabaseError, ex:
        logger.error('Failed to execute the statement on the database. Please, check log file for errors.')
        logger.error(ex)
        sys.exit(3)
    return

def execute_for_timing(dburl, func):
    conn = dbconn.connect(dburl)
    # Execute dummy command to bring up container
    cursor = dbconn.execSQL(conn, "select %s() from testdata limit 10" % func)
    cursor.fetchall()
    n1 = dt.datetime.now()
    cursor = dbconn.execSQL(conn, "select sum(%s()) from testdata" % func)
    cursor.fetchall()
    n2 = dt.datetime.now()
    cursor.close()
    conn.close()
    return ((n2-n1).seconds*1e6 + (n2-n1).microseconds) / 1e6

def main():
    dbURL = dbconn.DbURL(hostname = '127.0.0.1',
                         port     = 5432,
                         dbname   = 'pl_regression',
                         username = 'vagrant')

    for numrecs in [10000,20000,30000,40000,50000]:
        execute_noret(dbURL, "drop table if exists testdata")
        execute_noret(dbURL, "create table testdata (id int, a int) distributed by (id)")
        execute_noret(dbURL, "insert into testdata (id,a) select 1, id from generate_series(1,%d) id" % numrecs)
        for func in ['pylog100', 'upylog100']:
            s = 0.0
            for i in range(5):
                s += execute_for_timing (dbURL, func)
            s /= 5.0
            print '%s %d %f' % (func, numrecs, s)

main()
