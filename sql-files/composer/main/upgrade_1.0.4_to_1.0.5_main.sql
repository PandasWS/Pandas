
--
-- Pandas Table structure for table `suspend`
--

CREATE TABLE IF NOT EXISTS `suspend` (
  `account_id` int(11) unsigned NOT NULL,
  `char_id` int(10) unsigned NOT NULL,
  `sex` enum('F','M') NOT NULL DEFAULT 'M',
  `map` varchar(20) NOT NULL,
  `x` smallint(5) unsigned NOT NULL,
  `y` smallint(5) unsigned NOT NULL,
  `body_direction` CHAR( 1 ) NOT NULL DEFAULT '4',
  `head_direction` CHAR( 1 ) NOT NULL DEFAULT '0',
  `sit` CHAR( 1 ) NOT NULL DEFAULT '1',
  `mode` tinyint(4) NOT NULL,
  `tick` bigint(20) NOT NULL default '0',
  `val1` int(11) NOT NULL default '0',
  `val2` int(11) NOT NULL default '0',
  `val3` int(11) NOT NULL default '0',
  `val4` int(11) NOT NULL default '0',
  PRIMARY KEY (`account_id`)
) ENGINE=MyISAM;
