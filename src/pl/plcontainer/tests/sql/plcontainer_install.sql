DROP TYPE IF EXISTS plcontainer_status CASCADE;

CREATE TYPE plcontainer_status AS (
    segment_id int,
    status varchar
);

CREATE OR REPLACE FUNCTION plcontainer_read_local_config(verbose bool) RETURNS text
AS 'plcontainer', 'read_plcontainer_config'
LANGUAGE C VOLATILE;

CREATE OR REPLACE FUNCTION plcontainer_read_config(verbose bool) RETURNS SETOF plcontainer_status AS $$
    select gp_segment_id, plcontainer_read_local_config($1)
        from (
            select gp_segment_id
                from gp_dist_random('pg_namespace')
                group by 1
            ) as segments
    union all
    select -1, plcontainer_read_local_config($1);
$$ LANGUAGE SQL VOLATILE;

CREATE OR REPLACE FUNCTION plcontainer_read_config() RETURNS SETOF plcontainer_status AS $$
    select plcontainer_read_config(false);
$$ LANGUAGE SQL VOLATILE;