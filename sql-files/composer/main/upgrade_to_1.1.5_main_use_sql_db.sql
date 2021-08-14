-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.5 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- 
-- 若您在 inter_athena.conf 中启用了 use_sql_db 选项, 那么请导入本文件.
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20210629.sql & 熊猫模拟器自定义修改
-- -----------------------------------------------

-- 将价格字段的类型从 mediumint(8) 提高到 int(10) 以便提高金额上限
-- 若不调整则最大价格无法超过 8,388,607, 调整后最大可达到 2,147,483,647
-- 
-- 将 name_english 长度调整为 100 是 upgrade_20210629.sql 中的修改

DROP PROCEDURE IF EXISTS UPDATE_PANDAS_MAIN_USESQLDB;

DELIMITER $$
CREATE PROCEDURE UPDATE_PANDAS_MAIN_USESQLDB()
BEGIN
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db') THEN
		ALTER TABLE `item_db`
			MODIFY `name_english` varchar(100),
			MODIFY `price_buy` int(10) unsigned default NULL,
			MODIFY `price_sell` int(10) unsigned default NULL;
	END IF;

	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db_re') THEN
		ALTER TABLE `item_db_re`
			MODIFY `name_english` varchar(100),
			MODIFY `price_buy` int(10) unsigned default NULL,
			MODIFY `price_sell` int(10) unsigned default NULL;
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db2') THEN
		ALTER TABLE `item_db2`
			MODIFY `name_english` varchar(100),
			MODIFY `price_buy` int(10) unsigned default NULL,
			MODIFY `price_sell` int(10) unsigned default NULL;
	END IF;
	
	IF EXISTS (SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = (SELECT DATABASE ()) and TABLE_NAME = 'item_db2_re') THEN
		ALTER TABLE `item_db2_re`
			MODIFY `name_english` varchar(100),
			MODIFY `price_buy` int(10) unsigned default NULL,
			MODIFY `price_sell` int(10) unsigned default NULL;
	END IF;
END $$
DELIMITER ;

CALL UPDATE_PANDAS_MAIN_USESQLDB();
DROP PROCEDURE UPDATE_PANDAS_MAIN_USESQLDB;
