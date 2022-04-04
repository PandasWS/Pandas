-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.10 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20211230.sql
-- -----------------------------------------------

-- 熊猫模拟器优化对极端计算的支持 (AKA: 变态服拓展包)
-- 因此部分字段与 rAthena 官方设计相比需要调整数据库字段的类型

ALTER TABLE `char` 
	ADD COLUMN `pow` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `luk`,
	ADD COLUMN `sta` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `pow`,
	ADD COLUMN `wis` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `sta`,
	ADD COLUMN `spl` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `wis`,
	ADD COLUMN `con` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `spl`,
	ADD COLUMN `crt` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `con`,
	ADD COLUMN `max_ap` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `sp`,
	ADD COLUMN `ap` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `max_ap`,
	ADD COLUMN `trait_point` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `skill_point`
;

-- -----------------------------------------------
-- 修改背包拓展字段的字段名
-- -----------------------------------------------

ALTER TABLE `char`
	CHANGE `inventory_size` `inventory_slots` smallint(6) NOT NULL default '100'
;

-- -----------------------------------------------
-- upgrade_20220121.sql
-- -----------------------------------------------

CREATE TABLE IF NOT EXISTS `barter` (
  `name` varchar(50) NOT NULL DEFAULT '',
  `index` SMALLINT(5) UNSIGNED NOT NULL,
  `amount` SMALLINT(5) UNSIGNED NOT NULL,
  PRIMARY KEY  (`name`,`index`)
) ENGINE=MyISAM;

-- -----------------------------------------------
-- upgrade_20220204.sql
-- -----------------------------------------------

ALTER TABLE `market`
MODIFY `amount` INT(11) NOT NULL
;
