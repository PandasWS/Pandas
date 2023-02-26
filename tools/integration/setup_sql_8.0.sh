#!/usr/bin/env bash

function aborterror {
	echo $@
	exit 1
}

mysql --host $DB_HOST -u $DB_ROOT -e "CREATE DATABASE $DB_NAME;" || aborterror "Unable to create database."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/main.sql || aborterror "Unable to import main database."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/logs.sql || aborterror "Unable to import logs database."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db.sql || aborterror "Unable to import pre-renewal item table structure."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db_usable.sql || aborterror "Unable to import pre-renewal usable item table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db_equip.sql || aborterror "Unable to import pre-renewal equip item table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db_etc.sql || aborterror "Unable to import pre-renewal etc item table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db2.sql || aborterror "Unable to import pre-renewal item 2 table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db_re.sql || aborterror "Unable to import renewal item table structure."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db_re_usable.sql || aborterror "Unable to import renewal usable item table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db_re_equip.sql || aborterror "Unable to import renewal equip item table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db_re_etc.sql || aborterror "Unable to import renewal etc item table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/item_db2_re.sql || aborterror "Unable to import renewal item 2 table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/mob_db.sql || aborterror "Unable to import pre-renewal monster table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/mob_db2.sql || aborterror "Unable to import pre-renewal monster 2 table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/mob_db_re.sql || aborterror "Unable to import renewal monster table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/mob_db2_re.sql || aborterror "Unable to import renewal monster 2 table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/mob_skill_db.sql || aborterror "Unable to import pre-renewal monster skill table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/mob_skill_db2.sql || aborterror "Unable to import pre-renewal monster skill 2 table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/mob_skill_db_re.sql || aborterror "Unable to import renewal monster skill table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/mob_skill_db2_re.sql || aborterror "Unable to import renewal monster skill 2 table."
mysql --host $DB_HOST -u $DB_ROOT $DB_NAME < sql-files/roulette_default_data.sql || aborterror "Unable to import roulette table."

mysql --host $DB_HOST -u $DB_ROOT -e "CREATE USER '$DB_USER'@'%' IDENTIFIED WITH mysql_native_password BY '$DB_PASS';"
mysql --host $DB_HOST -u $DB_ROOT -e "GRANT ALL PRIVILEGES ON $DB_NAME.* TO'$DB_USER'@'%';"
