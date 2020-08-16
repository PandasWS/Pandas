-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.0.3 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20191222.sql
-- -----------------------------------------------

ALTER TABLE `bonus_script`
    ADD PRIMARY KEY (`char_id`, `type`);

ALTER TABLE `buyingstore_items`
    ADD PRIMARY KEY (`buyingstore_id`, `index`);

ALTER TABLE `friends`
    DROP INDEX `char_id`,
    ADD PRIMARY KEY (`char_id`, `friend_id`);

ALTER TABLE `interlog`
    ADD COLUMN `id` INT NOT NULL AUTO_INCREMENT FIRST,
    ADD PRIMARY KEY (`id`),
    ADD INDEX `time` (`time`);

ALTER TABLE `ipbanlist`
    DROP INDEX `list`,
    ADD PRIMARY KEY (`list`, `btime`);

ALTER TABLE `sc_data`
    DROP INDEX `account_id`,
    DROP INDEX `char_id`,
    ADD PRIMARY KEY (`char_id`, `type`);

ALTER TABLE `skillcooldown`
    DROP INDEX `account_id`,
    DROP INDEX `char_id`,
    ADD PRIMARY KEY (`char_id`, `skill`);

ALTER TABLE `vending_items`
    ADD PRIMARY KEY (`vending_id`, `index`);

DROP TABLE `sstatus`;

-- -----------------------------------------------
-- upgrade_20200108.sql
-- -----------------------------------------------

ALTER TABLE `charlog`
    -- DROP PRIMARY KEY, -- comment if primary key has not been created yet
    ADD COLUMN `id` bigint(20) unsigned NOT NULL auto_increment first,
    ADD PRIMARY KEY (`id`),
    ADD KEY `account_id` (`account_id`);

-- -----------------------------------------------
-- upgrade_20200109.sql
-- -----------------------------------------------

ALTER TABLE `acc_reg_num`
	MODIFY `value` bigint(11) NOT NULL default '0';

ALTER TABLE `global_acc_reg_num`
	MODIFY `value` bigint(11) NOT NULL default '0';

ALTER TABLE `char_reg_num`
	MODIFY `value` bigint(11) NOT NULL default '0';

-- -----------------------------------------------
-- upgrade_20200126.sql
-- -----------------------------------------------

ALTER TABLE `achievement`
	MODIFY `count1` int unsigned NOT NULL default '0',
	MODIFY `count2` int unsigned NOT NULL default '0',
	MODIFY `count3` int unsigned NOT NULL default '0',
	MODIFY `count4` int unsigned NOT NULL default '0',
	MODIFY `count5` int unsigned NOT NULL default '0',
	MODIFY `count6` int unsigned NOT NULL default '0',
	MODIFY `count7` int unsigned NOT NULL default '0',
	MODIFY `count8` int unsigned NOT NULL default '0',
	MODIFY `count9` int unsigned NOT NULL default '0',
	MODIFY `count10` int unsigned NOT NULL default '0';

-- -----------------------------------------------
-- upgrade_20200204.sql
-- -----------------------------------------------

ALTER TABLE `guild_member`
	DROP COLUMN `account_id`,
	DROP COLUMN `hair`,
	DROP COLUMN `hair_color`,
	DROP COLUMN `gender`,
	DROP COLUMN `class`,
	DROP COLUMN `lv`,
	DROP COLUMN `exp_payper`,
	DROP COLUMN `online`,
	DROP COLUMN `name`;

ALTER TABLE `friends`
	DROP COLUMN `friend_account`;

-- -----------------------------------------------
-- upgrade_20200303.sql
-- -----------------------------------------------

UPDATE `char_reg_num` SET `key` = 'ep14_3_newerabs' WHERE `key` = 'ep14_3_dimensional_travel' AND `index` = 0 AND `value` < 2;
UPDATE `char_reg_num` SET `key` = 'ep14_3_newerabs', `value` = 3 WHERE `key` = 'ep14_3_dimensional_travel' AND `index` = 0 AND `value` = 2;
UPDATE `char_reg_num` SET `key` = 'ep14_3_newerabs', `value` = `value` + 2 WHERE `key` = 'ep14_3_dimensional_travel' AND `index` = 0 AND `value` < 8;
UPDATE `char_reg_num` SET `key` = 'ep14_3_newerabs', `value` = `value` + 7 WHERE `key` = 'ep14_3_dimensional_travel' AND `index` = 0 AND `value` > 7;
