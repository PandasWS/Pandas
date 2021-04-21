-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.0 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20201105.sql
-- -----------------------------------------------

ALTER TABLE `auction`
	ADD COLUMN `enchantgrade` tinyint unsigned NOT NULL default '0';

ALTER TABLE `cart_inventory`
	ADD COLUMN `enchantgrade` tinyint unsigned NOT NULL default '0';

ALTER TABLE `guild_storage`
	ADD COLUMN `enchantgrade` tinyint unsigned NOT NULL default '0';

ALTER TABLE `guild_storage_log`
	ADD COLUMN `enchantgrade` tinyint unsigned NOT NULL default '0';

ALTER TABLE `inventory`
	ADD COLUMN `enchantgrade` tinyint unsigned NOT NULL default '0';

ALTER TABLE `mail_attachments`
	ADD COLUMN `enchantgrade` tinyint unsigned NOT NULL default '0';

ALTER TABLE `storage`
	ADD COLUMN `enchantgrade` tinyint unsigned NOT NULL default '0';
