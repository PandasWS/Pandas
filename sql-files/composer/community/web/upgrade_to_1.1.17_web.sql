-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的 WEB 接口数据库升级到 1.1.17 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

ALTER TABLE `guild_emblems`
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`world_name`, `guild_id`);

ALTER TABLE `user_configs`
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`world_name`, `account_id`);

ALTER TABLE `char_configs`
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`world_name`, `account_id`, `char_id`);

ALTER TABLE `merchant_configs`
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`world_name`, `account_id`, `char_id`, `store_type`);
