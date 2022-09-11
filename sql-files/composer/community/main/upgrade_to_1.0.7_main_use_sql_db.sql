-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.0.7 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- 
-- 若您在 inter_athena.conf 中启用了 use_sql_db 选项, 那么请导入本文件.
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20200808.sql - 已开启 use_sql_db 部分
-- -----------------------------------------------

DROP PROCEDURE IF EXISTS UPDATE_PANDAS_MAIN_USESQLDB;

DELIMITER $$
CREATE PROCEDURE UPDATE_PANDAS_MAIN_USESQLDB()
BEGIN
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_cash_db') THEN
		ALTER TABLE `item_cash_db`
			MODIFY `item_id` int(10) unsigned NOT NULL default '0';
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_cash_db2') THEN
		ALTER TABLE `item_cash_db2`
			MODIFY `item_id` int(10) unsigned NOT NULL default '0';
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db') THEN
		ALTER TABLE `item_db`
			MODIFY `id` int(10) unsigned NOT NULL default '0';
	END IF;

	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db_re') THEN
		ALTER TABLE `item_db_re`
			MODIFY `id` int(10) unsigned NOT NULL default '0';
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db2') THEN
		ALTER TABLE `item_db2`
			MODIFY `id` int(10) unsigned NOT NULL default '0';
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db2_re') THEN
		ALTER TABLE `item_db2_re`
			MODIFY `id` int(10) unsigned NOT NULL default '0';
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'mob_db') THEN
		ALTER TABLE `mob_db`
			MODIFY `MVP1id` int(10) unsigned NOT NULL default '0',
			MODIFY `MVP2id` int(10) unsigned NOT NULL default '0',
			MODIFY `MVP3id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop1id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop2id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop3id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop4id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop5id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop6id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop7id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop8id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop9id` int(10) unsigned NOT NULL default '0',
			MODIFY `DropCardid` int(10) unsigned NOT NULL default '0';
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'mob_db_re') THEN
		ALTER TABLE `mob_db_re`
			MODIFY `MVP1id` int(10) unsigned NOT NULL default '0',
			MODIFY `MVP2id` int(10) unsigned NOT NULL default '0',
			MODIFY `MVP3id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop1id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop2id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop3id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop4id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop5id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop6id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop7id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop8id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop9id` int(10) unsigned NOT NULL default '0',
			MODIFY `DropCardid` int(10) unsigned NOT NULL default '0';
	END IF;

	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'mob_db2') THEN
		ALTER TABLE `mob_db2`
			MODIFY `MVP1id` int(10) unsigned NOT NULL default '0',
			MODIFY `MVP2id` int(10) unsigned NOT NULL default '0',
			MODIFY `MVP3id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop1id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop2id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop3id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop4id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop5id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop6id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop7id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop8id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop9id` int(10) unsigned NOT NULL default '0',
			MODIFY `DropCardid` int(10) unsigned NOT NULL default '0';
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'mob_db2_re') THEN
		ALTER TABLE `mob_db2_re`
			MODIFY `MVP1id` int(10) unsigned NOT NULL default '0',
			MODIFY `MVP2id` int(10) unsigned NOT NULL default '0',
			MODIFY `MVP3id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop1id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop2id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop3id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop4id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop5id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop6id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop7id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop8id` int(10) unsigned NOT NULL default '0',
			MODIFY `Drop9id` int(10) unsigned NOT NULL default '0',
			MODIFY `DropCardid` int(10) unsigned NOT NULL default '0';
	END IF;
END $$
DELIMITER ;

CALL UPDATE_PANDAS_MAIN_USESQLDB();
DROP PROCEDURE UPDATE_PANDAS_MAIN_USESQLDB;
