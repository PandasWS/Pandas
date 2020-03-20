
-- upgrade_20180623.sql

ALTER TABLE `guild_position` MODIFY COLUMN `mode` smallint(11) unsigned NOT NULL default '0';

-- upgrade_20180830.sql

UPDATE `char` ch, `skill` sk SET `ch`.`skill_point` = `ch`.`skill_point` + `sk`.`lv` WHERE `sk`.`id` = 2049 AND `ch`.`char_id` = `sk`.`char_id`;

DELETE FROM `skill` WHERE `id` = 2049;

-- upgrade_20181010.sql

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

-- upgrade_20181220.sql

ALTER TABLE `bonus_script` MODIFY COLUMN `tick` bigint(20) NOT NULL default '0';

ALTER TABLE `elemental` MODIFY COLUMN `life_time` bigint(20) NOT NULL default '0';

ALTER TABLE `mercenary` MODIFY COLUMN `life_time` bigint(20) NOT NULL default '0';

ALTER TABLE `sc_data` MODIFY COLUMN `tick` bigint(20) NOT NULL;

ALTER TABLE `skillcooldown` MODIFY COLUMN `tick` bigint(20) NOT NULL;

-- upgrade_20181224.sql

alter table `inventory`
	add column `equip_switch` int(11) unsigned NOT NULL default '0' after `unique_id`;

-- upgrade_20190309.sql

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

-- upgrade_20190814.sql

ALTER TABLE `ipbanlist`
	CHANGE COLUMN `list` `list` VARCHAR(15) NOT NULL DEFAULT '' FIRST;

-- upgrade_20190815.sql

DROP TABLE `ragsrvinfo`;

-- upgrade_20191222.sql

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

-- upgrade_20200108.sql

ALTER TABLE `charlog`
    -- DROP PRIMARY KEY, -- comment if primary key has not been created yet
    ADD COLUMN `id` bigint(20) unsigned NOT NULL auto_increment first,
    ADD PRIMARY KEY (`id`),
    ADD KEY `account_id` (`account_id`);

-- upgrade_20200109.sql

ALTER TABLE `acc_reg_num`
	MODIFY `value` bigint(11) NOT NULL default '0';

ALTER TABLE `global_acc_reg_num`
	MODIFY `value` bigint(11) NOT NULL default '0';

ALTER TABLE `char_reg_num`
	MODIFY `value` bigint(11) NOT NULL default '0';

-- upgrade_20200126.sql

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

-- upgrade_20200204.sql

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

-- upgrade_20200303.sql

UPDATE `char_reg_num` SET `key` = 'ep14_3_newerabs' WHERE `key` = 'ep14_3_dimensional_travel' AND `index` = 0 AND `value` < 2;
UPDATE `char_reg_num` SET `key` = 'ep14_3_newerabs', `value` = 3 WHERE `key` = 'ep14_3_dimensional_travel' AND `index` = 0 AND `value` = 2;
UPDATE `char_reg_num` SET `key` = 'ep14_3_newerabs', `value` = `value` + 2 WHERE `key` = 'ep14_3_dimensional_travel' AND `index` = 0 AND `value` < 8;
UPDATE `char_reg_num` SET `key` = 'ep14_3_newerabs', `value` = `value` + 7 WHERE `key` = 'ep14_3_dimensional_travel' AND `index` = 0 AND `value` > 7;

-- 熊猫模拟器的额外修正, 感谢 "张大坏" 反馈

ALTER TABLE `bonus_script` MODIFY `tick` BIGINT(20) NOT NULL DEFAULT '0';
ALTER TABLE `elemental` MODIFY `life_time` BIGINT(20) NOT NULL default '0';
ALTER TABLE `mercenary` MODIFY `life_time` BIGINT(20) NOT NULL default '0';
ALTER TABLE `ipbanlist` MODIFY `list` VARCHAR(15) NOT NULL default '';
ALTER TABLE `sc_data` MODIFY `tick` BIGINT(20) NOT NULL;
ALTER TABLE `skillcooldown` MODIFY `tick` BIGINT(20) NOT NULL;
