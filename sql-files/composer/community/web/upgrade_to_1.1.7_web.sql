-- ------------------------------------------------------------------------------
-- 此脚本用于将 Pandas 的 WEB 接口数据库升级到 1.1.7 版本
-- 注意: 若存在更低版本且从未导入的升级脚本, 请按版本号从小到大依序导入
-- ------------------------------------------------------------------------------

ALTER TABLE `char_configs`
	ADD COLUMN `char_id` INT(11) UNSIGNED NOT NULL AFTER `account_id`,
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`account_id`, `char_id`, `world_name`);
DELETE FROM `char_configs` WHERE `char_id` = 0;

--
-- Table structure for table `merchant_configs`
--

CREATE TABLE IF NOT EXISTS `merchant_configs` (
  `account_id` int(11) unsigned NOT NULL,
  `char_id` INT(11) UNSIGNED NOT NULL,
  `world_name` varchar(32) NOT NULL,
  `store_type` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `data` longtext NOT NULL,
  PRIMARY KEY (`account_id`, `char_id`, `world_name`)
) ENGINE=MyISAM;

--
-- Table structure for table `recruitment`
--

CREATE TABLE IF NOT EXISTS `recruitment` (
  `account_id` int(11) unsigned NOT NULL,
  `char_id` INT(11) UNSIGNED NOT NULL,
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `world_name` varchar(32) NOT NULL,
  `adventure_type` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `tanker` tinyint(3) UNSIGNED NOT NULL DEFAULT 1,
  `dealer` tinyint(3) UNSIGNED NOT NULL DEFAULT 1,
  `healer` tinyint(3) UNSIGNED NOT NULL DEFAULT 1,
  `assist` tinyint(3) UNSIGNED NOT NULL DEFAULT 1,
  `min_level` INT(11) UNSIGNED NOT NULL,
  `max_level` INT(11) UNSIGNED NOT NULL,
  `memo` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`account_id`, `char_id`, `world_name`)
) ENGINE=MyISAM;
