-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.2.5 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20240803.sql
-- -----------------------------------------------

UPDATE `char_reg_num` SET `key` = 'ep18_main' WHERE `key` = 'ep18_1_main';

-- -----------------------------------------------
-- upgrade_20240914.sql
-- -----------------------------------------------

ALTER TABLE `guild_expulsion` ADD COLUMN `char_id` int(11) unsigned NOT NULL default '0';

-- -----------------------------------------------
-- upgrade_20241005.sql
-- -----------------------------------------------

ALTER TABLE `homunculus`
	CHANGE COLUMN `sp` `sp` INT(11) UNSIGNED NOT NULL DEFAULT '0',
	CHANGE COLUMN `max_sp` `max_sp` INT(11) UNSIGNED NOT NULL DEFAULT '0';

-- -----------------------------------------------
-- upgrade_20241216.sql
-- -----------------------------------------------

CREATE TABLE IF NOT EXISTS `skillcooldown_homunculus` (
  `homun_id` int(11) NOT NULL,
  `skill` smallint(11) unsigned NOT NULL DEFAULT '0',
  `tick` bigint(20) NOT NULL,
  PRIMARY KEY (`homun_id`)
) ENGINE=MyISAM;

CREATE TABLE IF NOT EXISTS `skillcooldown_mercenary` (
  `mer_id` int(11) NOT NULL,
  `skill` smallint(11) unsigned NOT NULL DEFAULT '0',
  `tick` bigint(20) NOT NULL,
  PRIMARY KEY (`mer_id`)
) ENGINE=MyISAM;

-- -----------------------------------------------
-- upgrade_20250126.sql
-- -----------------------------------------------

ALTER TABLE `char`
	ADD COLUMN `disable_partyinvite` tinyint(1) unsigned NOT NULL default '0' AFTER `disable_call`;

-- -----------------------------------------------
-- upgrade_20250201.sql
-- -----------------------------------------------

ALTER TABLE `skillcooldown_homunculus`
	DROP PRIMARY KEY,
	ADD PRIMARY KEY(`homun_id`, `skill`)
;

ALTER TABLE `skillcooldown_mercenary`
	DROP PRIMARY KEY,
	ADD PRIMARY KEY(`mer_id`, `skill`)
;
