
--
-- Table structure for table `guild_emblems`
--

CREATE TABLE IF NOT EXISTS `guild_emblems` (
  `guild_id` int(11) unsigned NOT NULL,
  `world_name` varchar(32) NOT NULL,
  `file_type` varchar(255) NOT NULL,
  `file_data` blob,
  `version` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY (`guild_id`, `world_name`)
) ENGINE=MyISAM;

--
-- Table structure for table `user_configs`
--

CREATE TABLE IF NOT EXISTS `user_configs` (
  `account_id` int(11) unsigned NOT NULL,
  `world_name` varchar(32) NOT NULL,
  `data` longtext NOT NULL,
  PRIMARY KEY (`account_id`, `world_name`)
) ENGINE=MyISAM;


--
-- Table structure for table `char_configs`
--

CREATE TABLE IF NOT EXISTS `char_configs` (
  `account_id` int(11) unsigned NOT NULL,
  `char_id` INT(11) UNSIGNED NOT NULL,	-- Pandas add this field for distinguish character data
  `world_name` varchar(32) NOT NULL,
  `data` longtext NOT NULL,
  PRIMARY KEY (`account_id`, `char_id`, `world_name`)	-- Pandas add char_id field into primary key
) ENGINE=MyISAM;

--
-- Table structure for table `merchant_configs`
--

CREATE TABLE IF NOT EXISTS `merchant_configs` (
  `account_id` int(11) unsigned NOT NULL,
  `char_id` INT(11) UNSIGNED NOT NULL,
  `world_name` varchar(32) NOT NULL,
  `store_type` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `data` longtext NOT NULL,
  PRIMARY KEY (`account_id`, `char_id`, `world_name`, `store_type`)
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
