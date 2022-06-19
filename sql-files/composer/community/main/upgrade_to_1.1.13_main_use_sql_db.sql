-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.13 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- 
-- 若您在 inter_athena.conf 中启用了 use_sql_db 选项, 那么请导入本文件.
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- 熊猫模拟器自定义修改
-- -----------------------------------------------

DROP PROCEDURE IF EXISTS UPDATE_PANDAS_MAIN_USESQLDB;

DELIMITER $$
CREATE PROCEDURE UPDATE_PANDAS_MAIN_USESQLDB()
BEGIN
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db_re') THEN
		ALTER TABLE `item_db_re`
			ADD COLUMN `job_spirit_handler` tinyint(1) unsigned DEFAULT NULL AFTER `job_soullinker`;
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db2_re') THEN
		ALTER TABLE `item_db2_re`
			ADD COLUMN `job_spirit_handler` tinyint(1) unsigned DEFAULT NULL AFTER `job_soullinker`;
	END IF;
END $$
DELIMITER ;

CALL UPDATE_PANDAS_MAIN_USESQLDB();
DROP PROCEDURE UPDATE_PANDAS_MAIN_USESQLDB;
