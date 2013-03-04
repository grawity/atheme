--- A PostgreSQL Schema for Atheme 0.2
--- Copyright (c) 2005 William Pitcock, et al.
---
--- Use of this code is permitted under the terms presented in doc/LICENSE.
---
--- $Id: atheme_init.pgsql 3241 2005-10-29 20:48:51Z alambert $

CREATE SEQUENCE accounts_id_seq;
CREATE SEQUENCE account_metadata_id_seq;
CREATE SEQUENCE account_memos_seq;
CREATE SEQUENCE account_memo_ignores_seq;
CREATE SEQUENCE channels_id_seq;
CREATE SEQUENCE channel_metadata_id_seq;
CREATE SEQUENCE channel_access_id_seq;
CREATE SEQUENCE channel_access_metadata_id_seq;
CREATE SEQUENCE klines_id_seq;

--- MU equivilant -- see db.c for explanation of the data structure.
CREATE TABLE ACCOUNTS
(
	ID		INT		DEFAULT nextval('accounts_id_seq')	NOT NULL,
	USERNAME	VARCHAR(255)	NOT NULL,
	PASSWORD	VARCHAR(255)	NOT NULL,
	EMAIL		VARCHAR(255)	NOT NULL,
	REGISTERED	INT		NOT NULL,
	LASTLOGIN	INT,
	FLAGS		INT
);

CREATE TABLE ACCOUNT_METADATA
(
	ID		INT		DEFAULT nextval('account_metadata_id_seq')	NOT NULL,
	PARENT		INT		NOT NULL,
	KEYNAME		TEXT		NOT NULL,
	VALUE		TEXT		NOT NULL
);

CREATE TABLE ACCOUNT_MEMOS
(
	ID		INT		DEFAULT nextval('account_memos_seq')	NOT NULL,
	PARENT		INT		NOT NULL,
	SENDER		VARCHAR(255)	NOT NULL,
	TIME		INT		NOT NULL,
	STATUS		INT,
	TEXT		TEXT		NOT NULL
);

CREATE TABLE ACCOUNT_MEMO_IGNORES
(
	ID		INT		DEFAULT nextval('account_memo_ignores_seq')	NOT NULL,
	PARENT		INT		NOT NULL,
	TARGET		VARCHAR(255)	NOT NULL
);

--- MC equivilant
CREATE TABLE CHANNELS
(
	ID		INT		DEFAULT nextval('channels_id_seq')	NOT NULL,
	NAME		VARCHAR(255)	NOT NULL,
	FOUNDER		VARCHAR(255)	NOT NULL,
	REGISTERED	INT		NOT NULL,
	LASTUSED	INT		NOT NULL,
	FLAGS		INT		NOT NULL,
	MLOCK_ON	INT		NOT NULL,
	MLOCK_OFF	INT		NOT NULL,
	MLOCK_LIMIT	INT		NOT NULL,
	MLOCK_KEY	VARCHAR(255)	NOT NULL
);

CREATE TABLE CHANNEL_METADATA
(
	ID		INT		DEFAULT nextval('channel_metadata_id_seq')	NOT NULL,
	PARENT		INT		NOT NULL,
	KEYNAME		TEXT		NOT NULL,
	VALUE		TEXT		NOT NULL
);

CREATE TABLE CHANNEL_ACCESS
(
	ID		INT		DEFAULT nextval('channel_access_id_seq')	NOT NULL,
	PARENT		INT		NOT NULL,
	ACCOUNT		VARCHAR(255)	NOT NULL,
	PERMISSIONS	VARCHAR(255)	NOT NULL
);

CREATE TABLE CHANNEL_ACCESS_METADATA
(
	ID		INT		DEFAULT nextval('channel_access_metadata_id_seq')	NOT NULL,
	PARENT		INT		NOT NULL,
	KEYNAME		TEXT		NOT NULL,
	VALUE		TEXT		NOT NULL
);

--- KL equivilant
CREATE TABLE KLINES
(
	ID		INT		DEFAULT nextval('klines_id_seq')	NOT NULL,
	USERNAME	VARCHAR(255)	NOT NULL,
	HOSTNAME	VARCHAR(255)	NOT NULL,
	DURATION	INT,
	SETTIME		INT		NOT NULL,
	SETTER		VARCHAR(255)	NOT NULL,
	REASON		TEXT		NOT NULL
);
