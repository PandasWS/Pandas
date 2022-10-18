-- ------------------------------------------------------------------------------
-- 此脚本仅用于将 rAthenaCN 1.8.0 的主数据库升级到 Pandas 1.0.0 版本
-- 导入此脚本之后, 若您在 rAthenaCN 的 inter_athena.conf 启用了 use_sql_db 选项,
-- 那么还需额外导入 upgrade_from_rAthenaCN-1.8.0_main_use_sql_db.sql 文件
-- 
-- 完成上述操作后, 您的 rAthenaCN 主数据库将会被升级到 Pandas 1.0.0 版本.
-- 随后, 你还需要按顺序导入 Pandas 历史各版本的主数据库升级文件, 如: 
-- 
-- 1. 导入 upgrade_to_1.0.3_main.sql 将主数据库升级到 Pandas 1.0.3 版本
-- 2. 导入 upgrade_to_1.0.5_main.sql 将主数据库升级到 Pandas 1.0.5 版本
-- 3. ....
-- 
-- 提示: 之所以没有 upgrade_to_1.0.1_main.sql 是因为 1.0.0 和 1.0.1 这两个版本的
--       数据库结构一致, 因此无需升级.
--       以此类推, 之所以没有 upgrade_to_1.0.4_main.sql 是因为 1.0.3 和 1.0.4 这
--       两个版本的数据库结构一致, 因此无需升级. 
--
-- 注意: 若发现存在命名规则如 upgrade_to_x.x.x_main_use_sql_db.sql 的文件
--       那么只有你开启了 use_sql_db 选项才需要导入它们.
--       导入的顺序为: 首先导入与版本对应的 upgrade_to_x.x.x_main.sql 文件,
--       再导入对应的 upgrade_to_x.x.x_main_use_sql_db.sql 文件.
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20180623.sql
-- -----------------------------------------------

ALTER TABLE `guild_position` MODIFY COLUMN `mode` smallint(11) unsigned NOT NULL default '0';

-- -----------------------------------------------
-- upgrade_20180830.sql
-- -----------------------------------------------

UPDATE `char` ch, `skill` sk SET `ch`.`skill_point` = `ch`.`skill_point` + `sk`.`lv` WHERE `sk`.`id` = 2049 AND `ch`.`char_id` = `sk`.`char_id`;

DELETE FROM `skill` WHERE `id` = 2049;

-- -----------------------------------------------
-- upgrade_20181010.sql
-- -----------------------------------------------

CREATE TABLE IF NOT EXISTS `guild_storage_log` (
  `id` int(11) NOT NULL auto_increment,
  `guild_id` int(11) unsigned NOT NULL default '0',
  `time` datetime NOT NULL,
  `char_id` int(11) NOT NULL default '0',
  `name` varchar(24) NOT NULL default '',
  `nameid` smallint(5) unsigned NOT NULL default '0',
  `amount` int(11) NOT NULL default '1',
  `identify` smallint(6) NOT NULL default '0',
  `refine` tinyint(3) unsigned NOT NULL default '0',
  `attribute` tinyint(4) unsigned NOT NULL default '0',
  `card0` smallint(5) unsigned NOT NULL default '0',
  `card1` smallint(5) unsigned NOT NULL default '0',
  `card2` smallint(5) unsigned NOT NULL default '0',
  `card3` smallint(5) unsigned NOT NULL default '0',
  `option_id0` smallint(5) NOT NULL default '0',
  `option_val0` smallint(5) NOT NULL default '0',
  `option_parm0` tinyint(3) NOT NULL default '0',
  `option_id1` smallint(5) NOT NULL default '0',
  `option_val1` smallint(5) NOT NULL default '0',
  `option_parm1` tinyint(3) NOT NULL default '0',
  `option_id2` smallint(5) NOT NULL default '0',
  `option_val2` smallint(5) NOT NULL default '0',
  `option_parm2` tinyint(3) NOT NULL default '0',
  `option_id3` smallint(5) NOT NULL default '0',
  `option_val3` smallint(5) NOT NULL default '0',
  `option_parm3` tinyint(3) NOT NULL default '0',
  `option_id4` smallint(5) NOT NULL default '0',
  `option_val4` smallint(5) NOT NULL default '0',
  `option_parm4` tinyint(3) NOT NULL default '0',
  `expire_time` int(11) unsigned NOT NULL default '0',
  `unique_id` bigint(20) unsigned NOT NULL default '0',
  `bound` tinyint(1) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  INDEX (`guild_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1;

-- -----------------------------------------------
-- upgrade_20181220.sql
-- -----------------------------------------------

ALTER TABLE `bonus_script` MODIFY COLUMN `tick` bigint(20) NOT NULL default '0';

ALTER TABLE `elemental` MODIFY COLUMN `life_time` bigint(20) NOT NULL default '0';

ALTER TABLE `mercenary` MODIFY COLUMN `life_time` bigint(20) NOT NULL default '0';

ALTER TABLE `sc_data` MODIFY COLUMN `tick` bigint(20) NOT NULL;

ALTER TABLE `skillcooldown` MODIFY COLUMN `tick` bigint(20) NOT NULL;

-- -----------------------------------------------
-- upgrade_20181224.sql
-- -----------------------------------------------

alter table `inventory`
	add column `equip_switch` int(11) unsigned NOT NULL default '0' after `unique_id`;

-- -----------------------------------------------
-- upgrade_20190309.sql
-- -----------------------------------------------

ALTER TABLE `pet`
	ADD COLUMN `autofeed` tinyint(2) NOT NULL default '0' AFTER `incubate`;

UPDATE `inventory` `i`
INNER JOIN `char` `c`
ON `i`.`char_id` = `c`.`char_id` AND `c`.`pet_id` <> '0'
SET `i`.`attribute` = '1'
WHERE
	`i`.`card0` = '256'
AND
	( `i`.`card1` | ( `i`.`card2` << 16 ) ) = `c`.`pet_id`
;

INSERT INTO `inventory`( `char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3` )
SELECT
	`p`.`char_id`,						-- Character ID
	`p`.`egg_id`,						-- Egg Item ID
	'1',								-- Amount
	'0',								-- Equip
	'1',								-- Identify
	'0',								-- Refine
	'1',								-- Attribute
	'256',								-- Card0
	( `p`.`pet_id` & 0xFFFF ),			-- Card1
	( ( `p`.`pet_id` >> 16 ) & 0xFFFF ),	-- Card2
	'0'									-- Card3
FROM `pet` `p`
LEFT JOIN `inventory` `i`
ON
	`i`.`char_id` = `p`.`char_id`
AND
	`i`.`nameid` = `p`.`egg_id`
AND
	`i`.`card0` = '256'
AND
	( `i`.`card1` | ( `i`.`card2` << 16 ) ) = `p`.`pet_id`
WHERE
	`p`.`incubate` = '0'
AND
	`i`.`id` IS NULL
;

-- -----------------------------------------------
-- upgrade_20190814.sql
-- -----------------------------------------------

ALTER TABLE `ipbanlist`
	CHANGE COLUMN `list` `list` VARCHAR(15) NOT NULL DEFAULT '' FIRST;

-- -----------------------------------------------
-- upgrade_20190815.sql
-- -----------------------------------------------

DROP TABLE `ragsrvinfo`;

-- -----------------------------------------------
-- 熊猫模拟器的额外修正, 感谢 "张大坏" 反馈
-- -----------------------------------------------

ALTER TABLE `bonus_script` MODIFY `tick` BIGINT(20) NOT NULL DEFAULT '0';
ALTER TABLE `elemental` MODIFY `life_time` BIGINT(20) NOT NULL default '0';
ALTER TABLE `mercenary` MODIFY `life_time` BIGINT(20) NOT NULL default '0';
ALTER TABLE `ipbanlist` MODIFY `list` VARCHAR(15) NOT NULL default '';
ALTER TABLE `sc_data` MODIFY `tick` BIGINT(20) NOT NULL;
ALTER TABLE `skillcooldown` MODIFY `tick` BIGINT(20) NOT NULL;
