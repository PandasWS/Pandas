-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.0.7 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- 
-- 导入此脚本之后, 若您在 inter_athena.conf 启用了 use_sql_db 选项,
-- 那么还需额外导入 upgrade_to_1.0.7_main_use_sql_db.sql 文件
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20200603.sql
-- -----------------------------------------------

ALTER TABLE  `char` ADD COLUMN `hotkey_rowshift2` TINYINT(3) UNSIGNED NOT NULL DEFAULT  '0' AFTER `hotkey_rowshift`;

-- -----------------------------------------------
-- upgrade_20200604.sql
-- -----------------------------------------------

UPDATE `char` `c`
INNER JOIN `login` `l`
ON `l`.`account_id` = `c`.`account_id`
SET `c`.`sex` = `l`.`sex`
WHERE
	`c`.`sex` = 'U'
AND
	`l`.`sex` <> 'S'
;

ALTER TABLE `char`
	MODIFY `sex` ENUM('M','F') NOT NULL
;

-- -----------------------------------------------
-- upgrade_20200622.sql
-- -----------------------------------------------

UPDATE `char_reg_num` SET `value` = `value` * 100 WHERE `key` = 'guildtime' AND `index` = 0 AND `value` < 24;

-- upgrade_20200625.sql

ALTER TABLE `login`
	ADD COLUMN `web_auth_token` VARCHAR(17) NULL AFTER `old_group`,
	ADD COLUMN `web_auth_token_enabled` tinyint(2) NOT NULL default '0' AFTER `web_auth_token`,
	ADD UNIQUE KEY `web_auth_token_key` (`web_auth_token`)
;

-- -----------------------------------------------
-- upgrade_20200703.sql
-- -----------------------------------------------

-- Fix rename flag and intimacy in inventories
update `inventory` `i`
inner join `pet` `p`
on
	`i`.`card0` = 256
and
	( `i`.`card1` | ( `i`.`card2` << 16 ) ) = `p`.`pet_id`
set
	`i`.`card3` = 
		( 
			CASE
				WHEN `p`.`intimate` < 100 THEN
					1 -- awkward
				WHEN `p`.`intimate` < 250 THEN
					2 -- shy
				WHEN `p`.`intimate` < 750 THEN
					3 -- neutral
				WHEN `p`.`intimate` < 910 THEN
					4 -- cordial
				WHEN `p`.`intimate` <= 1000 THEN
					5 -- loyal
				ELSE 0 -- unrecognized
			END << 1
		) | `p`.`rename_flag`
;

-- Fix rename flag and intimacy in carts
update `cart_inventory` `i`
inner join `pet` `p`
on
	`i`.`card0` = 256
and
	( `i`.`card1` | ( `i`.`card2` << 16 ) ) = `p`.`pet_id`
set
	`i`.`card3` = 
		( 
			CASE
				WHEN `p`.`intimate` < 100 THEN
					1 -- awkward
				WHEN `p`.`intimate` < 250 THEN
					2 -- shy
				WHEN `p`.`intimate` < 750 THEN
					3 -- neutral
				WHEN `p`.`intimate` < 910 THEN
					4 -- cordial
				WHEN `p`.`intimate` <= 1000 THEN
					5 -- loyal
				ELSE 0 -- unrecognized
			END << 1
		) | `p`.`rename_flag`
;

-- Fix rename flag and intimacy in storages
update `storage` `i`
inner join `pet` `p`
on
	`i`.`card0` = 256
and
	( `i`.`card1` | ( `i`.`card2` << 16 ) ) = `p`.`pet_id`
set
	`i`.`card3` = 
		( 
			CASE
				WHEN `p`.`intimate` < 100 THEN
					1 -- awkward
				WHEN `p`.`intimate` < 250 THEN
					2 -- shy
				WHEN `p`.`intimate` < 750 THEN
					3 -- neutral
				WHEN `p`.`intimate` < 910 THEN
					4 -- cordial
				WHEN `p`.`intimate` <= 1000 THEN
					5 -- loyal
				ELSE 0 -- unrecognized
			END << 1
		) | `p`.`rename_flag`
;

-- Fix rename flag and intimacy in guild storages
update `guild_storage` `i`
inner join `pet` `p`
on
	`i`.`card0` = 256
and
	( `i`.`card1` | ( `i`.`card2` << 16 ) ) = `p`.`pet_id`
set
	`i`.`card3` = 
		( 
			CASE
				WHEN `p`.`intimate` < 100 THEN
					1 -- awkward
				WHEN `p`.`intimate` < 250 THEN
					2 -- shy
				WHEN `p`.`intimate` < 750 THEN
					3 -- neutral
				WHEN `p`.`intimate` < 910 THEN
					4 -- cordial
				WHEN `p`.`intimate` <= 1000 THEN
					5 -- loyal
				ELSE 0 -- unrecognized
			END << 1
		) | `p`.`rename_flag`
;

-- -----------------------------------------------
-- upgrade_20200728.sql
-- -----------------------------------------------

ALTER TABLE `guild`
	CHANGE COLUMN `next_exp` `next_exp` bigint(20) unsigned NOT NULL default '0';

-- -----------------------------------------------
-- upgrade_20200808.sql - 未开启 use_sql_db 部分
-- -----------------------------------------------

ALTER TABLE `auction`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0',
	MODIFY `card0` int(10) unsigned NOT NULL default '0',
	MODIFY `card1` int(10) unsigned NOT NULL default '0',
	MODIFY `card2` int(10) unsigned NOT NULL default '0',
	MODIFY `card3` int(10) unsigned NOT NULL default '0';

ALTER TABLE `cart_inventory`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0',
	MODIFY `card0` int(10) unsigned NOT NULL default '0',
	MODIFY `card1` int(10) unsigned NOT NULL default '0',
	MODIFY `card2` int(10) unsigned NOT NULL default '0',
	MODIFY `card3` int(10) unsigned NOT NULL default '0';

ALTER TABLE `db_roulette`
	MODIFY `item_id` int(10) unsigned NOT NULL default '0';

ALTER TABLE `guild_storage`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0',
	MODIFY `card0` int(10) unsigned NOT NULL default '0',
	MODIFY `card1` int(10) unsigned NOT NULL default '0',
	MODIFY `card2` int(10) unsigned NOT NULL default '0',
	MODIFY `card3` int(10) unsigned NOT NULL default '0';

ALTER TABLE `guild_storage_log`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0',
	MODIFY `card0` int(10) unsigned NOT NULL default '0',
	MODIFY `card1` int(10) unsigned NOT NULL default '0',
	MODIFY `card2` int(10) unsigned NOT NULL default '0',
	MODIFY `card3` int(10) unsigned NOT NULL default '0';

ALTER TABLE `inventory`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0',
	MODIFY `card0` int(10) unsigned NOT NULL default '0',
	MODIFY `card1` int(10) unsigned NOT NULL default '0',
	MODIFY `card2` int(10) unsigned NOT NULL default '0',
	MODIFY `card3` int(10) unsigned NOT NULL default '0';

ALTER TABLE `mail_attachments`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0',
	MODIFY `card0` int(10) unsigned NOT NULL default '0',
	MODIFY `card1` int(10) unsigned NOT NULL default '0',
	MODIFY `card2` int(10) unsigned NOT NULL default '0',
	MODIFY `card3` int(10) unsigned NOT NULL default '0';

ALTER TABLE `market`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0';

ALTER TABLE `pet`
	MODIFY `egg_id` int(10) unsigned NOT NULL default '0',
	MODIFY `equip` int(10) unsigned NOT NULL default '0';

ALTER TABLE `sales`
	MODIFY `nameid` int(10) unsigned NOT NULL;

ALTER TABLE `storage`
	MODIFY `nameid` int(10) unsigned NOT NULL default '0',
	MODIFY `card0` int(10) unsigned NOT NULL default '0',
	MODIFY `card1` int(10) unsigned NOT NULL default '0',
	MODIFY `card2` int(10) unsigned NOT NULL default '0',
	MODIFY `card3` int(10) unsigned NOT NULL default '0';

-- -----------------------------------------------
-- upgrade_20200808b.sql
-- -----------------------------------------------

-- WL_SUMMONFB
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + (`s`.lv - 2), `s`.lv = 2 WHERE `s`.id = 2222 AND `s`.lv > 2 AND `c`.char_id = `s`.char_id;

-- WL_SUMMONBL
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + (`s`.lv - 2), `s`.lv = 2 WHERE `s`.id = 2223 AND `s`.lv > 2 AND `c`.char_id = `s`.char_id;

-- WL_SUMMONWB
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + (`s`.lv - 2), `s`.lv = 2 WHERE `s`.id = 2224 AND `s`.lv > 2 AND `c`.char_id = `s`.char_id;

-- WL_SUMMONSTONE
UPDATE `char` c, `skill` s SET `c`.skill_point = `c`.skill_point + (`s`.lv - 2), `s`.lv = 2 WHERE `s`.id = 2229 AND `s`.lv > 2 AND `c`.char_id = `s`.char_id;

-- -----------------------------------------------
-- upgrade_20200811.sql
-- -----------------------------------------------

-- Verus police quests
DELETE FROM `char_reg_num` WHERE `key` = 'trap_doom_prayers' AND `index` = 0;
DELETE FROM `char_reg_num` WHERE `key` = 'count_stone_seiden' AND `index` = 0;
-- Verus Wandering Bard quest
DELETE FROM `char_reg_num` WHERE `key` = 'wandering_bard_quest' AND `index` = 0;
-- To Phantasmagorika! quest
UPDATE `char_reg_num` SET `key` = 'ep15_1_elb' WHERE `key` = 'VER_ELEVATOR' AND `value` < 100 AND `index` = 0;
UPDATE `char_reg_num` SET `key` = 'ep15_1_elb', `value` = `value` - 94 WHERE `key` = 'VER_ELEVATOR' AND `value` >= 100 AND `value` < 1000 AND `index` = 0;
-- Vestige quest
UPDATE `char_reg_num` SET `key` = 'ep15_2_bslast', `value` = `value` - 999 WHERE `key` = 'VER_ELEVATOR' AND `value` >= 1000 AND `index` = 0;
-- Krotzel's Request quests
UPDATE `char_reg_num` SET `key` = 'ep15_2_brigan' WHERE `key` = 'VER_REPORTER' AND `index` = 0;
-- Main quest
INSERT INTO `char_reg_num` (`char_id`, `key`, `index`, `value`) SELECT `char_id`, 'ep15_2_permit', 0, 1 FROM `char_reg_num` WHERE `key` = 'VER_MAIN' AND `value` >= 19 AND `index` = 0;
UPDATE `char_reg_num` SET `key` = 'ep15_1_atnad' WHERE `key` = 'VER_MAIN' AND `value` < 31 AND `index` = 0;
UPDATE `char_reg_num` SET `key` = 'ep15_1_atnad', `value` = 30 WHERE `key` = 'VER_MAIN' AND `value` > 30 AND `value` < 37 AND `index` = 0;
UPDATE `char_reg_num` SET `key` = 'ep15_1_atnad', `value` = `value` - 6 WHERE `key` = 'VER_MAIN' AND `value` > 36 AND `index` = 0;
DELETE FROM `char_reg_num` WHERE `key` = 'VERUS_DAILY_QUEST' AND `index` = 0;
-- Memory quest
DELETE FROM `char_reg_num` WHERE `key` = 'recorder_quest_type' AND `index` = 0;
DELETE FROM `char_reg_num` WHERE `key` = 'recorder_quest_status' AND `index` = 0;

-- -----------------------------------------------
-- upgrade_20200821.sql
-- -----------------------------------------------

DELETE s FROM `skill` s, `char` c WHERE `s`.char_id = `c`.char_id AND (`c`.class = 4218 OR `c`.class = 4220) AND `c`.job_level > 60;
UPDATE `char` c SET `c`.job_level = 60, `c`.skill_point = 59 WHERE (`c`.class = 4218 OR `c`.class = 4220) AND `c`.job_level > 60;

