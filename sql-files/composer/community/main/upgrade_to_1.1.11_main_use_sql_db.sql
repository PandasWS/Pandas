-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.11 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- 
-- 若您在 inter_athena.conf 中启用了 use_sql_db 选项, 那么请导入本文件.
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20220224.sql
-- -----------------------------------------------

DROP PROCEDURE IF EXISTS UPDATE_PANDAS_MAIN_USESQLDB;

DELIMITER $$
CREATE PROCEDURE UPDATE_PANDAS_MAIN_USESQLDB()
BEGIN
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'mob_db') THEN
		ALTER TABLE `mob_db`
			ADD COLUMN `racegroup_malangdo` tinyint unsigned DEFAULT NULL;
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'mob_db2') THEN
		ALTER TABLE `mob_db2`
			ADD COLUMN `racegroup_malangdo` tinyint unsigned DEFAULT NULL;
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'mob_db_re') THEN
		ALTER TABLE `mob_db_re`
			ADD COLUMN `racegroup_malangdo` tinyint unsigned DEFAULT NULL;
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'mob_db2_re') THEN
		ALTER TABLE `mob_db2_re`
			ADD COLUMN `racegroup_malangdo` tinyint unsigned DEFAULT NULL;
	END IF;
END $$
DELIMITER ;

CALL UPDATE_PANDAS_MAIN_USESQLDB();
DROP PROCEDURE UPDATE_PANDAS_MAIN_USESQLDB;
