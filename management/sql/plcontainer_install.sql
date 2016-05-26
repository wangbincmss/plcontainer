-- Creating PL/Container trusted language

CREATE OR REPLACE FUNCTION plcontainer_call_handler()
RETURNS LANGUAGE_HANDLER
AS '$libdir/plcontainer' LANGUAGE C;

CREATE TRUSTED LANGUAGE plcontainer HANDLER plcontainer_call_handler;

-- Defining container configuration management functions

CREATE TYPE plcontainer_status AS (
    segment_id int,
    status varchar
);

CREATE OR REPLACE FUNCTION plcontainer_read_local_config(verbose bool) RETURNS text
AS '$libdir/plcontainer', 'read_plcontainer_config'
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