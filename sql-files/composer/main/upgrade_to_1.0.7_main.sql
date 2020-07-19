
-- upgrade_20200603.sql

ALTER TABLE  `char` ADD COLUMN `hotkey_rowshift2` TINYINT(3) UNSIGNED NOT NULL DEFAULT  '0' AFTER `hotkey_rowshift`;

-- upgrade_20200604.sql

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

-- upgrade_20200622.sql

UPDATE `char_reg_num` SET `value` = `value` * 100 WHERE `key` = 'guildtime' AND `index` = 0 AND `value` < 24;

-- upgrade_20200625.sql

ALTER TABLE `login`
	ADD COLUMN `web_auth_token` VARCHAR(17) NULL AFTER `old_group`,
	ADD COLUMN `web_auth_token_enabled` tinyint(2) NOT NULL default '0' AFTER `web_auth_token`,
	ADD UNIQUE KEY `web_auth_token_key` (`web_auth_token`)
;

-- upgrade_20200703.sql

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
