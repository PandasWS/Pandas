-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的主数据库升级到 1.1.19 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

-- -----------------------------------------------
-- upgrade_20221218.sql
-- -----------------------------------------------
-- 这个脚本使用过于复杂, 暂时不汇入

-- -----------------------------------------------
-- upgrade_20221220.sql
-- -----------------------------------------------

CREATE TABLE IF NOT EXISTS `party_bookings` (
  `world_name` varchar(32) NOT NULL,
  `account_id` int(11) unsigned NOT NULL,
  `char_id` int(11) unsigned NOT NULL,
  `char_name` varchar(30) NOT NULL,	-- Pandas modify for sync to `char` table field length
  `purpose` smallint(5) unsigned NOT NULL DEFAULT '0',
  `assist` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `damagedealer` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `healer` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `tanker` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `minimum_level` smallint(5) unsigned NOT NULL,
  `maximum_level` smallint(5) unsigned NOT NULL,
  `comment` varchar(255) NOT NULL DEFAULT '',
  `created` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`world_name`, `account_id`, `char_id`)
) ENGINE=MyISAM;
