CREATE DATABASE IF NOT EXIST Atheme;
USE Atheme;

; MU equivilant -- see db.c for explanation of the data structure.
CREATE TABLE accounts 
(
	ID		INT		NOT_NULL,
	USERNAME	VARCHAR		NOT_NULL,
	PASSWORD	VARCHAR		NOT_NULL,
);

; MC equivilant here

