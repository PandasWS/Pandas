-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.2 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20210201.sql
-- -----------------------------------------------

INSERT IGNORE INTO `sc_data` (`account_id`, `char_id`, `type`, `tick`) SELECT `account_id`, `char_id`, '752', '-1' FROM `char` WHERE `option` & '4194304' != '0';

-- -----------------------------------------------
-- upgrade_20210308.sql
-- -----------------------------------------------

-- 16.1 official variables
UPDATE `char_reg_num` SET `key` = 'ep16_royal' WHERE `key` = 'banquet_main_quest' AND `value` < 16;
UPDATE `char_reg_num` SET `key` = 'ep16_royal', `value` = `value` - 1 WHERE `key` = 'banquet_main_quest' AND `value` < 20;
UPDATE `char_reg_num` SET `key` = 'ep16_royal', `value` = `value` - 2 WHERE `key` = 'banquet_main_quest' AND `value` < 26;
UPDATE `char_reg_num` SET `key` = 'ep16_royal', `value` = 25 WHERE `key` = 'banquet_main_quest' AND `value` > 25;

DELETE FROM `char_reg_num` WHERE `key` = 'banquet_nerius_quest';
DELETE FROM `char_reg_num` WHERE `key` = 'banquet_heine_quest';
DELETE FROM `char_reg_num` WHERE `key` = 'banquet_richard_quest';

UPDATE `char_reg_num` SET `key` = 'ep16_wal' WHERE `key` = 'banquet_walther_quest' AND `value` < 2;
UPDATE `char_reg_num` SET `key` = 'ep16_wal', `value` = `value` - 1 WHERE `key` = 'banquet_walther_quest' AND `value` > 1;
UPDATE `char_reg_num` SET `key` = 'ep16_lug' WHERE `key` = 'banquet_roegenburg_quest';
UPDATE `char_reg_num` SET `key` = 'ep16_gaobs' WHERE `key` = 'banquet_geoborg_quest';

UPDATE `char_reg_num` SET `key` = 'ep16_wig' WHERE `key` = 'banquet_wigner_quest' AND `value` < 5;
UPDATE `char_reg_num` SET `key` = 'ep16_wig', `value` = `value` + 5 WHERE `key` = 'banquet_wigner_quest' AND `value` > 5;
UPDATE `char_reg_num` c, `quest` q SET c.`key` = 'ep16_wig', c.`value` = 10
WHERE c.`key` = 'banquet_wigner_quest' AND c.`value` = 5 AND q.`quest_id` = 14482 AND q.`state` = 1;
UPDATE `char_reg_num` c, `quest` q SET c.`key` = 'ep16_wig', c.`value` = 9
WHERE c.`key` = 'banquet_wigner_quest' AND c.`value` = 5 AND q.`quest_id` = 14480 AND q.`state` = 2;
UPDATE `char_reg_num` c, `quest` q SET c.`key` = 'ep16_wig', c.`value` = 8
WHERE c.`key` = 'banquet_wigner_quest' AND c.`value` = 5 AND q.`quest_id` = 14480 AND q.`state` = 1;
UPDATE `char_reg_num` c, `quest` q SET c.`key` = 'ep16_wig', c.`value` = 7
WHERE c.`key` = 'banquet_wigner_quest' AND c.`value` = 5 AND q.`quest_id` = 14481 AND q.`state` = 2;
UPDATE `char_reg_num` c, `quest` q SET c.`key` = 'ep16_wig', c.`value` = 6
WHERE c.`key` = 'banquet_wigner_quest' AND c.`value` = 5 AND q.`quest_id` = 14481 AND q.`state` = 1;
UPDATE `char_reg_num` SET `key` = 'ep16_wig' WHERE `key` = 'banquet_wigner_quest' AND `value` = 5;

UPDATE `char_reg_num` SET `key` = 'ep16_cookbs' WHERE `key` = 'banquet_quest_cooking' AND `value` < 3;
UPDATE `char_reg_num` SET `key` = 'ep16_cookbs', `value` = 2 WHERE `key` = 'banquet_quest_cooking' AND `value` = 3;
UPDATE `char_reg_num` SET `key` = 'ep16_cookbs', `value` = `value` + 8 WHERE `key` = 'banquet_quest_cooking' AND `value` > 3;

DELETE FROM `quest` WHERE `quest_id` = 11428;
DELETE FROM `quest` WHERE `quest_id` = 11429;
DELETE FROM `quest` WHERE `quest_id` = 11430;
DELETE FROM `quest` WHERE `quest_id` = 11431;

DELETE FROM `char_reg_num` WHERE `key` = 'banquet_quest_sauce';

-- -----------------------------------------------
-- upgrade_20210331.sql
-- -----------------------------------------------

UPDATE `inventory` SET `card2` = `card2` & 65535 WHERE `card0` = 254 OR `card0` = 255;
UPDATE `cart_inventory` SET `card2` = `card2` & 65535 WHERE `card0` = 254 OR `card0` = 255;
UPDATE `storage` SET `card2` = `card2` & 65535 WHERE `card0` = 254 OR `card0` = 255;
UPDATE `guild_storage` SET `card2` = `card2` & 65535 WHERE `card0` = 254 OR `card0` = 255;

-- -----------------------------------------------
-- 熊猫模拟器自定义修改
-- -----------------------------------------------

-- 使 char 可记录角色背包容量上限
ALTER TABLE `char`
	ADD COLUMN `inventory_size` smallint(6) unsigned NOT NULL default '100';

