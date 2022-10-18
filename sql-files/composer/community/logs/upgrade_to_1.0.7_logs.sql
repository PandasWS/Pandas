-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的日志数据库升级到 1.0.7 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20200808_logs.sql
-- -----------------------------------------------

ALTER TABLE `feedinglog`
	MODIFY `item_id` int(10) unsigned NOT NULL default '0';

ALTER TABLE `mvplog`
	MODIFY `prize` int(10) unsigned NOT NULL default '0';

ALTER TABLE `picklog`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0',
	MODIFY `card0` int(10) unsigned NOT NULL default '0',
	MODIFY `card1` int(10) unsigned NOT NULL default '0',
	MODIFY `card2` int(10) unsigned NOT NULL default '0',
	MODIFY `card3` int(10) unsigned NOT NULL default '0';
