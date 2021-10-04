-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的 WEB 接口数据库升级到 1.1.7 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

ALTER TABLE `char_configs`
	ADD COLUMN `char_id` INT(11) UNSIGNED NOT NULL AFTER `account_id`,
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`account_id`, `char_id`, `world_name`);

DELETE FROM `char_configs` WHERE `char_id` = 0;

