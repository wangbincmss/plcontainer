
CREATE TABLE users (
	fname text not null,
	lname text not null,
	username text,
	userid serial
	) ;

CREATE TABLE taxonomy (
	id serial,
	name text
	) ;

CREATE TABLE entry (
	accession text not null,
	eid serial,
	txid int2 not null
	) ;

CREATE TABLE sequences (
	eid int4 not null,
	pid serial,
	product text not null,
	sequence text not null,
	multipart bool default 'false'
	) ;

CREATE TABLE xsequences (
	pid int4 not null,
	sequence text not null
	) ;

CREATE TABLE unicode_test (
	testvalue  text NOT NULL
);

CREATE TABLE table_record (
	first text,
	second int4
	) ;

CREATE TYPE type_record AS (
	first text,
	second int4
	) ;
